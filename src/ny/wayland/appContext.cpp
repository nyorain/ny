// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/appContext.hpp>

#include <ny/wayland/util.hpp>
#include <ny/wayland/interfaces.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/input.hpp>
#include <ny/wayland/data.hpp>
#include <ny/wayland/bufferSurface.hpp>
#include <ny/wayland/xdg-shell-client-protocol.h>

#include <ny/loopControl.hpp>
#include <ny/log.hpp>

#ifdef NY_WithEgl
 #include <ny/common/egl.hpp>
 #include <ny/wayland/egl.hpp>
#endif //WithGL

#ifdef NY_WithVulkan
 #define VK_USE_PLATFORM_WAYLAND_KHR
 #include <ny/wayland/vulkan.hpp>
 #include <vulkan/vulkan.h>
#endif //WithVulkan

#include <nytl/scope.hpp>

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-client-protocol.h>

#include <poll.h>
#include <unistd.h>
#include <sys/eventfd.h>

#include <algorithm>
#include <mutex>
#include <queue>
#include <cstring>

namespace ny
{

namespace
{

///Wayland LoopInterface implementation.
///The WaylandAppContext uses a modified version of wl_display_dispatch that
///additionally listens for various fds. One of them is an eventfd that is triggered every
///time .stop or .call is called to wake the loop up.
///The class has additionally an atomic bool that will be set to false when the
///loop was stopped.
///The eventfd is only used to wake the polling up, it does not transmit any useful information.
class WaylandLoopImpl : public ny::LoopInterfaceGuard
{
public:
	std::atomic<bool> run {true};

	unsigned int eventfd {};
	std::queue<std::function<void()>> functions {};
	std::mutex mutex {};

public:
	WaylandLoopImpl(LoopControl& lc, unsigned int evfd)
		: LoopInterfaceGuard(lc), eventfd(evfd)
	{
	}

	bool stop() override
	{
		run.store(0);
		std::int64_t v = 1;
		::write(eventfd, &v, 8);
		return true;
	};

	bool call(const std::function<void()>& function) override
	{
		if(!function) return false;

		{
			std::lock_guard<std::mutex> lock(mutex);
			functions.push(function);
		}

		std::int64_t v = 1;
		::write(eventfd, &v, 8);
		return true;
	}

	std::function<void()> popFunction()
	{
		std::lock_guard<std::mutex> lock(mutex);
		if(functions.empty()) return {};
		auto ret = functions.front();
		functions.pop();
		return ret;
	}
};

///Like poll but does not return on signals.
int noSigPoll(pollfd& fds, nfds_t nfds, int timeout = -1)
{
	int ret = 0;
	while(true)
	{
		ret = poll(&fds, nfds, timeout);
		if(ret != -1 || errno != EINTR) return ret;
	}
}

///Listener entry to implement custom fd polling callbacks in WaylandAppContext.
struct ListenerEntry
{
	int fd {};
	unsigned int events {};
	std::function<void(nytl::ConnectionRef, int fd, unsigned int events)> callback;
};

}

struct WaylandAppContext::Impl
{
	//here becuase wayland::NamedGlobal is in wayland/util.hpp
	wayland::NamedGlobal<wl_compositor> wlCompositor;
	wayland::NamedGlobal<wl_subcompositor> wlSubcompositor;
	wayland::NamedGlobal<wl_shell> wlShell;
	wayland::NamedGlobal<wl_shm> wlShm;
	wayland::NamedGlobal<wl_data_device_manager> wlDataManager;
	wayland::NamedGlobal<wl_seat> wlSeat;
	wayland::NamedGlobal<xdg_shell> xdgShell;

	//here because ConnectionList is in wayland/util.hpp
	ConnectionList<ListenerEntry> fdCallbacks;

	//here because changed is const functions (more like cache vars)
	std::vector<std::unique_ptr<WaylandErrorCategory>> errorCategories; //see WaylandEC for info
	std::error_code error; //The cached error code for the display (if any)

	#ifdef NY_WithEgl
		EglSetup eglSetup;
		bool eglFailed {}; //set to true if egl init failed, will not be tried again
	#endif //WithEGL
};

using namespace wayland;

WaylandAppContext::WaylandAppContext()
{
	//listeners
	using WAC = WaylandAppContext;
	constexpr static wl_registry_listener registryListener = {
		memberCallback<decltype(&WAC::handleRegistryAdd), &WAC::handleRegistryAdd,
			void(wl_registry*, uint32_t, const char*, uint32_t)>,
		memberCallback<decltype(&WAC::handleRegistryRemove), &WAC::handleRegistryRemove,
			void(wl_registry*, uint32_t)>,
	};

	constexpr static wl_shm_listener shmListener = {
		memberCallback<decltype(&WAC::handleShmFormat), &WAC::handleShmFormat,
			void(wl_shm*, uint32_t)>
	};

	constexpr static wl_seat_listener seatListener = {
		memberCallback<decltype(&WAC::handleSeatCapabilities), &WAC::handleSeatCapabilities,
			void(wl_seat*, uint32_t)>,
		memberCallback<decltype(&WAC::handleSeatName), &WAC::handleSeatName,
			void(wl_seat*, const char*)>
	};

	constexpr static xdg_shell_listener xdgShellListener = {
		memberCallback<decltype(&WAC::handleXdgShellPing), &WAC::handleXdgShellPing,
			void(xdg_shell*, uint32_t)>
	};

	//connect to the wayland displa and retrieve the needed objects
	impl_ = std::make_unique<Impl>();
	wlDisplay_ = wl_display_connect(nullptr);

	if(!wlDisplay_)
	{
		std::string msg = "ny::WaylandAppContext: could not connect to display";
		msg += strerror(errno);
		throw std::runtime_error(msg);
	}

	wlRegistry_ = wl_display_get_registry(wlDisplay_);
	wl_registry_add_listener(wlRegistry_, &registryListener, this);

	//roundtrip to assure that we receive all advertised globals
	wl_display_dispatch(wlDisplay_);
	wl_display_roundtrip(wlDisplay_);

	//compositor added by registry Callback listener
	//note that if it is not there now it simply does not exist on the server since
	//we rountripped above
	if(!impl_->wlCompositor)
		throw std::runtime_error("ny::WaylandAppContext: could not get compositor");

	//create eventfd needed to wake up polling
	//register a fd poll callback that just resets the eventfd and sets wakeup_
	//to true which will cause no further polling, but running the next loop
	//iteration in dispatchLoop
	eventfd_ = eventfd(0, EFD_NONBLOCK);
	fdCallback(eventfd_, POLLIN, [](int fd){
		int64_t v;
		read(fd, &v, 8);
	});

	//warn if features are missing
	if(!wlSeat()) warning("ny::WaylandAppContext: no wl_seat received, therefore no input events");
	if(!wlSubcompositor()) warning("ny::WaylandAppContext: no wl_subcomposirot recevied");
	if(!wlShell() && !xdgShell()) warning("ny::WaylandAppContext: no xdg or wl_shell received");
	if(!wlShm()) warning("ny::WaylandAppContext: no wl_shm recevied");
	if(!wlDataManager()) warning("ny::WaylandAppContext: no wl_data_manager received");

	//init secondary resources
	if(wlSeat())
	{
		wl_seat_add_listener(wlSeat(), &seatListener, this);
		if(wlDataManager()) dataDevice_ = std::make_unique<WaylandDataDevice>(*this);
	}

	if(wlShm())
	{
		wl_shm_add_listener(wlShm(), &shmListener, this);
		wlCursorTheme_ = wl_cursor_theme_load("default", 32, wlShm());
	}

	if(xdgShell()) xdg_shell_add_listener(xdgShell(), &xdgShellListener, this);

	//again roundtrip and dispatch pending events to finish all setup
	wl_display_dispatch(wlDisplay_);
	wl_display_roundtrip(wlDisplay_);
	wl_display_dispatch_pending(wlDisplay_);
}

WaylandAppContext::~WaylandAppContext()
{
	//we explicitly have to destroy/disconnect everything since wayland is plain c
	//note that additional (even RAII) members might have to be reset here too if there
	//destructor require the wayland display (or anything else) to be valid
	//therefor, we e.g. explicitly reset the egl unique ptrs
	outputs_.clear();
	impl_.reset();

	if(eventfd_) close(eventfd_);
	if(wlCursorTheme_) wl_cursor_theme_destroy(wlCursorTheme_);

	if(dataDevice_) dataDevice_.reset();
	if(keyboardContext_) keyboardContext_.reset();
	if(mouseContext_) mouseContext_.reset();

	if(xdgShell()) xdg_shell_destroy(xdgShell());
	if(wlShell()) wl_shell_destroy(wlShell());
	if(wlSeat()) wl_seat_destroy(wlSeat());
	if(wlDataManager()) wl_data_device_manager_destroy(wlDataManager());
	if(wlShm()) wl_shm_destroy(wlShm());
	if(wlSubcompositor()) wl_subcompositor_destroy(wlSubcompositor());
	if(impl_->wlCompositor) wl_compositor_destroy(&wlCompositor());

	if(wlRegistry_) wl_registry_destroy(wlRegistry_);
	if(wlDisplay_) wl_display_disconnect(wlDisplay_);
}

bool WaylandAppContext::dispatchEvents()
{
	if(!checkErrorWarn()) return false;

	dispatching_ = true;
	auto dispatchingGuard = nytl::makeScopeGuard([&]{ dispatching_ = false; });

	//first execute all pending dispatchers
	for(auto& dispatcher : pendingDispatchers_) dispatcher();
	pendingDispatchers_.clear();

	//read all registered file descriptors without any blocking and without blocking
	//for the display
	pollFds(0, 0);

	//dispatch all pending wayland events
	while(wl_display_prepare_read(wlDisplay_) == -1)
		wl_display_dispatch_pending(wlDisplay_);

	if(wl_display_flush(wlDisplay_) == -1)
	{
		wl_display_cancel_read(wlDisplay_);

		//if errno is eagain we have to wait until the compositor has read data
		//here, we just return since we don't want to block
		//otherwise handle the error
		if(errno != EAGAIN)
		{
			auto msg = std::strerror(errno);
			warning("ny::WaylandAppContext::dispatchEvents: wl_display_flush: ", msg);
		}
	}
	else
	{
		wl_display_read_events(wlDisplay_);
		wl_display_dispatch_pending(wlDisplay_);
	}

	return checkErrorWarn();
}

bool WaylandAppContext::dispatchLoop(LoopControl& control)
{
	if(!checkErrorWarn()) return false;
	WaylandLoopImpl loopImpl(control, eventfd_);

	dispatching_ = true;
	auto dispatchingGuard = nytl::makeScopeGuard([&]{ dispatching_ = false; });

	//first execute all pending dispatchers
	//we don't have to do this inside the loop because we set dispatching_
	//to true and therefore they will be dispatched directly
	for(auto& dispatcher : pendingDispatchers_) dispatcher();
	pendingDispatchers_.clear();

	while(loopImpl.run.load())
	{
		//call pending callback & dispatch functions
		while(auto func = loopImpl.popFunction()) func();

		if(dispatchDisplay()) continue;
		if(checkErrorWarn()) return false;
	}

	return checkErrorWarn();
}

KeyboardContext* WaylandAppContext::keyboardContext()
{
	return waylandKeyboardContext();
}

MouseContext* WaylandAppContext::mouseContext()
{
	return waylandMouseContext();
}

WindowContextPtr WaylandAppContext::createWindowContext(const WindowSettings& settings)
{
	WaylandWindowSettings waylandSettings;
	const auto* ws = dynamic_cast<const WaylandWindowSettings*>(&settings);

	if(ws) waylandSettings = *ws;
	else waylandSettings.WindowSettings::operator=(settings);

	if(settings.surface == SurfaceType::vulkan)
	{
		#ifdef NY_WithVulkan
			return std::make_unique<WaylandVulkanWindowContext>(*this, waylandSettings);
		#else
			static constexpr auto noVulkan = "ny::WaylandAppContext::createWindowContext: "
				"ny was built without vulkan support and can not create a Vulkan surface";

			throw std::logic_error(noVulkan);
		#endif
	}
	else if(settings.surface == SurfaceType::gl)
	{
		#ifdef NY_WithGl
			static constexpr auto eglFailed = "ny::WaylandAppContext::createWindowContext: "
				"initializing egl failed, therefore no gl surfaces can be created";

			if(!eglSetup()) throw std::runtime_error(eglFailed);
			return std::make_unique<WaylandEglWindowContext>(*this, *eglSetup(), waylandSettings);
		#else
			static constexpr auto noEgl = "ny::WaylandAppContext::createWindowContext: "
				"ny was built without gl/egl support and can therefore not create a Gl Surface";

			throw std::logic_error(noEgl);
		#endif
	}
	else if(settings.surface == SurfaceType::buffer)
	{
		return std::make_unique<WaylandBufferWindowContext>(*this, waylandSettings);
	}

	return std::make_unique<WaylandWindowContext>(*this, waylandSettings);
}

bool WaylandAppContext::clipboard(std::unique_ptr<DataSource>&& dataSource)
{
	if(!waylandDataDevice()) return false;

	auto src = new WaylandDataSource(*this, std::move(dataSource), false);
	wl_data_device_set_selection(&dataDevice_->wlDataDevice(), &src->wlDataSource(),
		keyboardContext_->lastSerial());
	return true;
}

DataOffer* WaylandAppContext::clipboard()
{
	if(!waylandDataDevice()) return nullptr;
	return waylandDataDevice()->clipboardOffer();
}

bool WaylandAppContext::startDragDrop(std::unique_ptr<DataSource>&& dataSource)
{
	if(!waylandMouseContext() || waylandDataDevice()) return false;

	auto over = mouseContext_->over();
	if(!over) return false;

	//see wayland/data.hpp WaylandDataSource documentation for a reason why <new> is used here.
	//this is not a leak, WaylandDataSource self-manages its lifetime
	auto src = new WaylandDataSource(*this, std::move(dataSource), true);
	auto surf = &static_cast<WaylandWindowContext*>(over)->wlSurface();
	wl_data_device_start_drag(&dataDevice_->wlDataDevice(), &src->wlDataSource(), surf, nullptr,
		mouseContext_->lastSerial());

	return true;
}

std::vector<const char*> WaylandAppContext::vulkanExtensions() const
{
	#ifdef NY_WithVulkan
		return {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
	#else
		return {};
	#endif
}

GlSetup* WaylandAppContext::glSetup() const
{
	#ifdef NY_WithEgl
		return eglSetup();
	#else
		return nullptr;
	#endif //Egl
}

EglSetup* WaylandAppContext::eglSetup() const
{
	#ifdef NY_WithEgl
		if(impl_->eglFailed) return nullptr;

		if(!impl_->eglSetup.valid())
		{
			try { impl_->eglSetup = {static_cast<void*>(&wlDisplay())}; }
			catch(const std::exception& error)
			{
				warning("ny::WaylandAppContext::eglSetup: init failed: ", error.what());
				impl_->eglFailed = true;
				impl_->eglSetup = {};
				return nullptr;
			}
		}

		return &impl_->eglSetup;

	#else
		return nullptr;

	#endif
}

std::error_code WaylandAppContext::checkError() const
{
	//We cache the error so we don't have to query it every time
	//This function is called/checked very often (critical paths)
	if(impl_->error) return impl_->error;

	auto err = wl_display_get_error(wlDisplay_);
	if(!err) return {};

	//when wayland returns this error code, we can query the exact interface
	//that triggered the error
	if(err == EPROTO)
	{
		const wl_interface* interface;
		uint32_t id;
		int code = wl_display_get_protocol_error(wlDisplay_, &interface, &id);

		//find or insert the matching category
		//see wayland/util.hpp WaylandErrorCategory for more information
		for(auto& category : impl_->errorCategories)
			if(&category->interface() == interface) return {code, *category};

		impl_->errorCategories.push_back(std::make_unique<WaylandErrorCategory>(*interface));
		impl_->error = {code, *impl_->errorCategories.back()};
	}
	else if(err)
	{
		impl_->error = {err, std::system_category()};
	}

	return impl_->error;
}

bool WaylandAppContext::checkErrorWarn() const
{
	auto ec = checkError();
	if(!ec) return true;

	auto msg = ec.message();
	error("ny::WaylandAppContext: display has error ", msg, " and should no longer be used");
	return false;
}

void WaylandAppContext::dispatch(std::function<void()> func)
{
	if(dispatching_) func();
	pendingDispatchers_.push_back(std::move(func));
}

void WaylandAppContext::dispatch(Event&& event)
{
	if(!event.handler)
	{
		warning("ny::WaylandAppContext::dispatch(Event): invalid event handler");
		return;
	}

	//this is ugly af!
	//problem: std::function is pendingDispatchers_ must be copyable, Event (and therefore
	//the lambda) is not.
	//But since this function is a hack anyways and will be removed with the event handling
	//rework, it's ok like this for now...
	auto sharedEvent = std::shared_ptr<Event>(nytl::cloneMove(event));
	pendingDispatchers_.push_back([=]{ sharedEvent->handler->handleEvent(*sharedEvent); });
}

nytl::Connection WaylandAppContext::fdCallback(int fd, unsigned int events,
	const FdCallbackFunc& func)
{
	return impl_->fdCallbacks.add({fd, events, func});
}

bool WaylandAppContext::dispatchDisplay()
{
	//In parts taken from wayland-client.c and modified to poll for the wayland fd as well as an
	//additional events. The wayland license when the source code was taken:
	//
	// Copyright © 2008-2012 Kristian Høgsberg
	// Copyright © 2010-2012 Intel Corporation
	//
	// Permission is hereby granted, free of charge, to any person obtaining
	// a copy of this software and associated documentation files (the
	// "Software"), to deal in the Software without restriction, including
	// without limitation the rights to use, copy, modify, merge, publish,
	// distribute, sublicense, and/or sell copies of the Software, and to
	// permit persons to whom the Software is furnished to do so, subject to
	// the following conditions:
	//
	// The above copyright notice and this permission notice (including the
	// next paragraph) shall be included in all copies or substantial
	// portions of the Software.
	//
	// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	// NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
	// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
	// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
	// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	// SOFTWARE.

	wakeup_ = false;
	int ret;

	if(wl_display_prepare_read(wlDisplay_) == -1)
		return wl_display_dispatch_pending(wlDisplay_);

	//try to flush the display until all data is flushed
	while(true) {
		ret = wl_display_flush(wlDisplay_);
		if(ret != -1 || errno != EAGAIN) break;

		//poll until the display could be written again.
		if(pollFds(POLLOUT, -1) == -1) {
			wl_display_cancel_read(wlDisplay_);
			return false;
		}

		//if polling exited since the eventfd was triggered, cancel the
		//wayland read
		if(wakeup_) {
			wl_display_cancel_read(wlDisplay_);
			return true;
		}
	}

	//Don't stop if flushing hits an EPIPE; continue so we can read any
	//protocol error that may have triggered it.
	if(ret < 0 && errno != EPIPE) {
		wl_display_cancel_read(wlDisplay_);
		return -1;
	}

	//poll for server events (and since this might block for fd callbacks)
	if(pollFds(POLLIN, -1) == -1) {
		wl_display_cancel_read(wlDisplay_);
		return -1;
	}

	//if dpypoll stopped due to the eventfd, cancel the wayland read
	if(wakeup_) {
		wl_display_cancel_read(wlDisplay_);
		return 0;
	}

	if(wl_display_read_events(wlDisplay_) == -1) return -1;
	return wl_display_dispatch_pending(wlDisplay_);
}

int WaylandAppContext::pollFds(short wlDisplayEvents, int timeout)
{
	//We first create a vector in which we tie a pfdindex with a connection id.
	//Then we poll for all file descriptors and iterate through the pfds/ties and try
	//to find the matching callback function for a connection id.
	//This has to be done this complex since the callback functions may actually disconnect
	//itself or other connections, i.e. fdCallbacks_ may change.
	auto& fdCallbacks = impl_->fdCallbacks;
	struct Tie
	{
		unsigned int pfdIndex;
		nytl::ConnectionID id;
	};

	std::vector<pollfd> fds;
	std::vector<Tie> ties;
	fds.reserve(fdCallbacks.items.size() + 1);
	ties.reserve(fdCallbacks.items.size());
	for(auto i = 0u; i < fdCallbacks.items.size(); ++i)
	{
		auto& fdc = fdCallbacks.items[i];
		fds.push_back({fdc.fd, static_cast<short>(fdc.events)});
		ties.push_back({i, fdc.clID_});
	}

	if(wlDisplayEvents) fds.push_back({wl_display_get_fd(wlDisplay_), wlDisplayEvents});

	auto ret = noSigPoll(*fds.data(), fds.size(), timeout);
	if(ret < 0)
	{
		log("ny::WaylandAppContext::pollFds: poll failed: ", std::strerror(errno));
		return ret;
	}

	for(auto pfdsi = 0u; pfdsi < fds.size(); ++pfdsi)
	{
		if(!fds[pfdsi].revents) continue;
		for(auto& callback : fdCallbacks.items)
		{
			if(callback.clID_ != ties[pfdsi].id) continue;
			nytl::ConnectionRef connref(fdCallbacks, callback.clID_);
			callback.callback(connref, fds[pfdsi].fd, fds[pfdsi].revents);
			break;
		}
	}

	return ret;
}

void WaylandAppContext::handleRegistryAdd(unsigned int id, const char* cinterface,
	unsigned int version)
{
	nytl::unused(version); //TODO
	const nytl::StringParam interface = cinterface; //easier comparison

	if(interface == "wl_compositor" && !impl_->wlCompositor)
	{
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_compositor_interface, 1);
		impl_->wlCompositor = {static_cast<wl_compositor*>(ptr), id};
	}
	else if(interface == "wl_shell" && !wlShell())
	{
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_shell_interface, 1);
		impl_->wlShell = {static_cast<wl_shell*>(ptr), id};
	}
	else if(interface == "wl_shm" && !wlShm())
	{
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_shm_interface, 1);
		impl_->wlShm = {static_cast<wl_shm*>(ptr), id};
	}
	else if(interface == "wl_subcompositor" && !impl_->wlSubcompositor)
	{
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_subcompositor_interface, 1);
		impl_->wlSubcompositor = {static_cast<wl_subcompositor*>(ptr), id};
	}
	else if(interface == "wl_output")
	{
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_output_interface, 2);
		outputs_.emplace_back(*this, *static_cast<wl_output*>(ptr), id);
	}
	else if(interface == "wl_data_device_manager" && !impl_->wlDataManager)
	{
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_data_device_manager_interface, 3);
		impl_->wlDataManager = {static_cast<wl_data_device_manager*>(ptr), id};
	}
	else if(interface == "wl_seat" && !impl_->wlSeat)
	{
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_seat_interface, 5);
		impl_->wlSeat = {static_cast<wl_seat*>(ptr), id};
	}
	else if(interface == "xdg_shell" && !impl_->xdgShell)
	{
		auto ptr = wl_registry_bind(&wlRegistry(), id, &xdg_shell_interface, 5);
		impl_->xdgShell = {static_cast<xdg_shell*>(ptr), id};
	}
}

void WaylandAppContext::handleRegistryRemove(unsigned int id)
{
	//TODO: stop the application/main loop when removing needed global? at least handle it somehow.
	//TODO: check other globals here?
	if(id == impl_->wlCompositor.name)
	{
		wl_compositor_destroy(&wlCompositor());
		impl_->wlCompositor = {};
	}
	else
	{
		outputs_.erase(std::remove_if(outputs_.begin(), outputs_.end(),
			[=](const Output& output){ return output.name() == id; }), outputs_.end());
	}
}

void WaylandAppContext::handleSeatCapabilities(unsigned int caps)
{
	//mouse
	if((caps & WL_SEAT_CAPABILITY_POINTER) && !mouseContext_)
	{
		mouseContext_ = std::make_unique<WaylandMouseContext>(*this, *wlSeat());
	}
	else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && mouseContext_)
	{
		log("ny::WaylandAppContext: lost wl_pointer");
		mouseContext_.reset();
	}

	//keyboard
	if((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !keyboardContext_)
	{
		keyboardContext_ = std::make_unique<WaylandKeyboardContext>(*this, *wlSeat());
	}
	else if(!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && keyboardContext_)
	{
		log("ny::WaylandAppContext: lost wl_keyboard");
		keyboardContext_.reset();
	}
}

void WaylandAppContext::handleSeatName(const char* name)
{
	seatName_ = name;
}

void WaylandAppContext::handleShmFormat(unsigned int format)
{
	shmFormats_.push_back(format);
}

void WaylandAppContext::handleXdgShellPing(unsigned int serial)
{
	if(!xdgShell()) return;
	xdg_shell_pong(xdgShell(), serial);
}

bool WaylandAppContext::shmFormatSupported(unsigned int wlShmFormat)
{
	for(auto format : shmFormats_) if(format == wlShmFormat) return true;
	return false;
}

WaylandWindowContext* WaylandAppContext::windowContext(wl_surface& surface) const
{
	auto data = wl_surface_get_user_data(&surface);
	return static_cast<WaylandWindowContext*>(data);
}

wl_pointer* WaylandAppContext::wlPointer() const
{
	if(!waylandMouseContext()) return nullptr;
	return waylandMouseContext()->wlPointer();
}

wl_keyboard* WaylandAppContext::wlKeyboard() const
{
	if(!waylandKeyboardContext()) return nullptr;
	return waylandKeyboardContext()->wlKeyboard();
}

wl_display& WaylandAppContext::wlDisplay() const { return *wlDisplay_; };
wl_registry& WaylandAppContext::wlRegistry() const { return *wlRegistry_; };
wl_compositor& WaylandAppContext::wlCompositor() const { return *impl_->wlCompositor; };
wl_subcompositor* WaylandAppContext::wlSubcompositor() const{ return impl_->wlSubcompositor; };
wl_shm* WaylandAppContext::wlShm() const { return impl_->wlShm; };
wl_seat* WaylandAppContext::wlSeat() const { return impl_->wlSeat; };
wl_shell* WaylandAppContext::wlShell() const { return impl_->wlShell; };
xdg_shell* WaylandAppContext::xdgShell() const { return impl_->xdgShell; }
wl_data_device_manager* WaylandAppContext::wlDataManager() const { return impl_->wlDataManager; }
wl_cursor_theme* WaylandAppContext::wlCursorTheme() const { return wlCursorTheme_; }

}
