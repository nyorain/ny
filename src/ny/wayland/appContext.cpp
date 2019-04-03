// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/appContext.hpp>

#include <ny/wayland/util.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/input.hpp>
#include <ny/wayland/dataExchange.hpp>
#include <ny/wayland/bufferSurface.hpp>

#include <ny/wayland/protocols/xdg-shell-unstable-v6.h>
#include <ny/wayland/protocols/xdg-shell.h>

#include <dlg/dlg.hpp>

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
#include <sstream>
#include <atomic>
#include <charconv>

// TODO: some globals are assumed to be present throughout code base.
// e.g. wl_shm, some functions (like WaylandWindowContext::cursor) will
// implicitly throw an error if it doesn't exist (-> wayland::ShmBuffer).
// there are probably some others. seat?
// either fix the errors (and replace them with warnings/propagated
// errors) or fail AppContext initializion if globals are not present.

// At the moment, we implement xdg_shell versions 5 and 6 since some compositors only
// support one of them. WaylandWindowContext will use version 6 is available.
// Later on, all support for version 5 might be dropped

namespace ny {
namespace {

// Like poll but does not return on signals.
int noSigPoll(pollfd& fds, nfds_t nfds, int timeout = -1) {
	while(true) {
		auto ret = poll(&fds, nfds, timeout);
		if(ret != -1 || errno != EINTR) return ret;
	}
}

// wl_log handler function
// Just outputs the log to ny::log and caches the last message in a threadlocal variable for an
// AppContext to be outputted in case a critical wayland error occurrs.
thread_local std::string lastLogMessage = "<none>";

void logHandler(const char* format, va_list vlist) {
	va_list vlistcopy;
	va_copy(vlistcopy, vlist);

	auto size = std::vsnprintf(nullptr, 0, format, vlist);
	va_end(vlist);

	lastLogMessage.resize(size + 1);
	std::vsnprintf(&lastLogMessage[0], lastLogMessage.size(), format, vlistcopy);
	lastLogMessage.pop_back(); // null-terminator
	lastLogMessage.pop_back(); // newline
	va_end(vlistcopy);

	dlg_debug("wayland log: {}", lastLogMessage);
}

// Listener entry to implement custom fd polling callbacks in WaylandAppContext.
struct ListenerEntry {
	int fd {};
	unsigned int events {};
	std::function<bool(int fd, unsigned int events)> callback;
};

} // anonymous util namespace

// NamesGlobal values are defined in impl because they need wayland/util.hpp
// Furthermore may there be additional globals in future versions of this implementation
struct WaylandAppContext::Impl {
	wayland::NamedGlobal<wl_compositor> wlCompositor;
	wayland::NamedGlobal<wl_subcompositor> wlSubcompositor;
	wayland::NamedGlobal<wl_shell> wlShell;
	wayland::NamedGlobal<wl_shm> wlShm;
	wayland::NamedGlobal<wl_data_device_manager> wlDataManager;
	wayland::NamedGlobal<wl_seat> wlSeat;
	wayland::NamedGlobal<zxdg_shell_v6> xdgShellV6;

	// here because ConnectionList is in wayland/util.hpp
	ConnectionList<ListenerEntry> fdCallbacks;

	#ifdef NY_WithEgl
		EglSetup eglSetup;
		bool eglFailed {}; // set to true if egl init failed, will not be tried again
	#endif //WithEGL
};

WaylandAppContext::WaylandAppContext() {
	// listeners
	using WAC = WaylandAppContext;
	constexpr static wl_registry_listener registryListener = {
		memberCallback<&WAC::handleRegistryAdd>,
		memberCallback<&WAC::handleRegistryRemove>
	};

	// connect to the wayland displa and retrieve the needed objects
	impl_ = std::make_unique<Impl>();
	wlDisplay_ = wl_display_connect(nullptr);

	if(!wlDisplay_) {
		std::string msg = "ny::WaylandAppContext(): could not connect to display";
		msg += strerror(errno);
		throw std::runtime_error(msg);
	}

	// set the wayland display log listener
	// we output the logged output to ny::log and additionally cache the last message
	// to add it to an error message if an error occurs.
	// since the log handler is not bound to a display we simply set it once and then just use
	// a threadlocal variable to cache the last message
	static std::once_flag onceFlag;
	std::call_once(onceFlag, []{
		wl_log_set_handler_client(logHandler);
	});

	// create the queue on which we do roundtrips.
	// note that we create an extra queue for roundtrips because they should not
	// dispatch any other events
	wlRoundtripQueue_ = wl_display_create_queue(&wlDisplay());

	// create registry with listener and retrieve globals
	wlRegistry_ = wl_display_get_registry(wlDisplay_);
	wl_registry_add_listener(wlRegistry_, &registryListener, this);

	// first dispatch to receive the registry events
	// then roundtrip to make sure binding them does not create an error
	// only here we can call the plain dispatch and roundtrip functions since there
	// aren't any applications callbacks yet
	wl_display_dispatch(wlDisplay_);
	wl_display_roundtrip(wlDisplay_);

	// compositor added by registry Callback listener
	// note that if it is not there now it simply does not exist on the server since
	// we rountripped above
	if(!impl_->wlCompositor) {
		throw std::runtime_error("ny::WaylandAppContext(): could not get compositor");
	}

	// create eventfd needed for wakeupWait
	eventfd_ = eventfd(0, EFD_NONBLOCK);
	fdCallback(eventfd_, POLLIN, [&](int, unsigned int){
		int64_t v;
		read(eventfd_, &v, 8);
		wakeup_ = true;
		return true;
	});

	// warn if features are missing
	if(!wlSeat()) dlg_warn("wl_seat not available, no input events");
	if(!wlSubcompositor()) dlg_warn("wl_subcompositor not available");
	if(!wlShm()) dlg_warn("wl_shm not available");
	if(!wlDataManager()) dlg_warn("wl_data_manager not available");
	if(!wlShell() && !xdgShellV6()) dlg_warn("no supported shell available");

	// init secondary resources
	if(wlSeat() && wlDataManager()) dataDevice_ = std::make_unique<WaylandDataDevice>(*this);
	if(wlShm()) {
		auto cursorTheme = std::getenv("XCURSOR_THEME");
        auto cursorSizeStr = std::getenv("XCURSOR_SIZE");
		auto cursorSize = 32u;
        if(cursorSizeStr) {
			auto end = cursorSizeStr + std::strlen(cursorSizeStr);
			unsigned parsed;
			auto res = std::from_chars(cursorSizeStr, end, parsed);
			if(res.ec == std::errc()) {
				cursorSize = parsed;
			}
        }

		// if XCURSOR_THEME is not set, we pass in null, which will result
		// in the default cursor theme being used.
		wlCursorTheme_ = wl_cursor_theme_load(cursorTheme, cursorSize, wlShm());
	}

	// again roundtrip and dispatch pending events to finish all setup and make
	// sure no errors occurred
	wl_display_roundtrip(wlDisplay_);
	wl_display_dispatch_pending(wlDisplay_);
}

WaylandAppContext::~WaylandAppContext() {
	// we explicitly have to destroy/disconnect everything since wayland is plain c
	// note that additional (even RAII) members might have to be reset here too if there
	// destructor require the wayland display (or anything else) to be valid
	// therefor, we e.g. explicitly reset the egl unique ptrs
	if(eventfd_) close(eventfd_);
	if(wlCursorTheme_) wl_cursor_theme_destroy(wlCursorTheme_);
	if(wlRoundtripQueue_) wl_event_queue_destroy(wlRoundtripQueue_);

	dataDevice_.reset();
	keyboardContext_.reset();
	mouseContext_.reset();

	clipboardSource_.reset();
	dndSource_.reset();

	if(xdgShellV6()) zxdg_shell_v6_destroy(xdgShellV6());

	if(wlShell()) wl_shell_destroy(wlShell());
	if(wlSeat()) wl_seat_destroy(wlSeat());
	if(wlDataManager()) wl_data_device_manager_destroy(wlDataManager());
	if(wlShm()) wl_shm_destroy(wlShm());
	if(wlSubcompositor()) wl_subcompositor_destroy(wlSubcompositor());
	if(impl_->wlCompositor) wl_compositor_destroy(&wlCompositor());

	impl_.reset();

	if(wlRegistry_) wl_registry_destroy(wlRegistry_);
	if(wlDisplay_) wl_display_disconnect(wlDisplay_);
}

void WaylandAppContext::pollEvents() {
	checkError();
	deferred.execute();

	// read all registered file descriptors without any blocking and without polling
	// for the display file descriptor, since we dispatch everything available anyways
	pollFds(0, 0);

	// dispatch all pending wayland events
	while(wl_display_prepare_read(wlDisplay_) == -1) {
		wl_display_dispatch_pending(wlDisplay_);
	}

	if(wl_display_flush(wlDisplay_) == -1) {
		// TODO: handle error correctly, canacel read if needed
		// wl_display_cancel_read(wlDisplay_);

		// if errno is eagain we have to wait until the compositor has read data
		// here, we just return since we don't want to block
		// otherwise handle the error
		if(errno != EAGAIN) {
			auto msg = std::strerror(errno);
			dlg_warn("wl_display_flush: {}", msg);
		}

		// if errno == EAGAIN we should poll the fd so it can be written
	} else {
		if(wl_display_read_events(wlDisplay_) == -1) {
			dlg_warn("wl_display_read_events: {}", std::strerror(errno));
		}
		wl_display_dispatch_pending(wlDisplay_);
	}

	deferred.execute();
	checkError();
}

void WaylandAppContext::waitEvents() {
	checkError();
	deferred.execute();
	dispatchDisplay();
	deferred.execute();
	checkError();
}

void WaylandAppContext::wakeupWait() {
	std::int64_t v = 1;
	::write(eventfd_, &v, 8);
}

KeyboardContext* WaylandAppContext::keyboardContext() {
	return waylandKeyboardContext();
}

MouseContext* WaylandAppContext::mouseContext() {
	return waylandMouseContext();
}

WindowContextPtr WaylandAppContext::createWindowContext(const WindowSettings& settings) {
	static const std::string func = "ny::WaylandAppContext::createWindowContext: ";
	WaylandWindowSettings waylandSettings;
	const auto* ws = dynamic_cast<const WaylandWindowSettings*>(&settings);

	if(ws) {
		waylandSettings = *ws;
	} else {
		waylandSettings.WindowSettings::operator=(settings);
	}

	if(settings.surface == SurfaceType::vulkan) {
		#ifdef NY_WithVulkan
			return std::make_unique<WaylandVulkanWindowContext>(*this, waylandSettings);
		#else
			throw std::logic_error(func + "ny was built without vulkan support");
		#endif
	} else if(settings.surface == SurfaceType::gl) {
		#ifdef NY_WithEgl
			if(!eglSetup()) {
				throw std::runtime_error(func + "initializing egl failed");
			}

			return std::make_unique<WaylandEglWindowContext>(*this, *eglSetup(), waylandSettings);
		#else
			throw std::logic_error(func + "ny was built without egl support")
		#endif
	} else if(settings.surface == SurfaceType::buffer) {
		return std::make_unique<WaylandBufferWindowContext>(*this, waylandSettings);
	}

	return std::make_unique<WaylandWindowContext>(*this, waylandSettings);
}

bool WaylandAppContext::clipboard(std::unique_ptr<DataSource>&& dataSource) {
	if(!waylandDataDevice()) {
		return false;
	}

	clipboardSource_ = std::make_unique<WaylandDataSource>(*this,
		std::move(dataSource), false);
	wl_data_device_set_selection(&dataDevice_->wlDataDevice(),
		&clipboardSource_->wlDataSource(), keyboardContext_->lastSerial());
	return true;
}

DataOffer* WaylandAppContext::clipboard() {
	if(!waylandDataDevice()) {
		return nullptr;
	}
	return waylandDataDevice()->clipboardOffer();
}

bool WaylandAppContext::dragDrop(const EventData* event,
		std::unique_ptr<DataSource>&& dataSource) {
	if(!waylandMouseContext() || !waylandDataDevice()) {
		return false;
	}

	auto wevent = dynamic_cast<const WaylandEventData*>(event);
	auto serial = wevent ? wevent->serial : mouseContext_->lastSerial();

	auto over = mouseContext_->over();
	if(!over) {
		return false;
	}

	try {
		dndSource_ = std::make_unique<WaylandDataSource>(*this, std::move(dataSource), true);
	} catch(const std::exception& error) {
		dlg_warn("WaylandDataSource threw: {}", error.what());
		return false;
	}

	auto surf = &static_cast<WaylandWindowContext*>(over)->wlSurface();
	wl_data_device_start_drag(&dataDevice_->wlDataDevice(),
		&dndSource_->wlDataSource(), surf, dndSource_->dragSurface(), serial);

	// only now we can attach a buffer to the surface since now it has the
	// dnd surface role.
	dndSource_->drawSurface();
	return true;
}

std::vector<const char*> WaylandAppContext::vulkanExtensions() const {
	#ifdef NY_WithVulkan
		return {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
	#else
		return {};
	#endif // WithVulkan
}

GlSetup* WaylandAppContext::glSetup() const {
	#ifdef NY_WithEgl
		return eglSetup();
	#else
		return nullptr;
	#endif // WithEgl
}

EglSetup* WaylandAppContext::eglSetup() const {
	#ifdef NY_WithEgl
		if(impl_->eglFailed) return nullptr;

		if(!impl_->eglSetup.valid()) {
			try {
				impl_->eglSetup = {static_cast<void*>(&wlDisplay())};
			} catch(const std::exception& error) {
				dlg_warn("initialization failed: {}", error.what());
				impl_->eglFailed = true;
				impl_->eglSetup = {};
				return nullptr;
			}
		}

		return &impl_->eglSetup;

	#else
		return nullptr;

	#endif // WithEgl
}

void WaylandAppContext::checkError() const {
	auto err = wl_display_get_error(wlDisplay_);
	if(!err) {
		return;
	}

	// when wayland returns this error code, we can query the exact interface
	// that triggered the error
	if(err == EPROTO) {
		const wl_interface* interface;
		uint32_t id;
		int code = wl_display_get_protocol_error(wlDisplay_, &interface, &id);

		auto errorName = "<unknown>";
		auto interfaceName = "<null interface>";
		if(interface) {
			errorName = wayland::errorName(*interface, code);
			interfaceName = interface->name;
		}

		auto msg = dlg::format(
			"ny::WaylandAppContext: display has critical protocol error\n\t",
			"error: '{}'\n\t"
			"interface: '{}'\n\t"
			"Last log output in this thread: '{}'\n\t"
			"This is likely a ny or compositor bug, please report it",
			errorName, interfaceName, lastLogMessage);

		throw BackendError(msg);
	}

	const char* errorName = std::strerror(err);
	if(!errorName) {
		errorName = "<unknown>";
	}

	auto msg = dlg::format(
		"Wayland display has non-protocol error '{}'\n\t"
		"Last log output in this thread: '{}'",
		errorName, lastLogMessage);
	throw BackendError(msg);
}

nytl::Connection WaylandAppContext::fdCallback(int fd, unsigned int events,
		const FdCallbackFunc& func) {
	return impl_->fdCallbacks.add({fd, events, func});
}

void WaylandAppContext::destroyDataSource(const WaylandDataSource& src) {
	if(&src == dndSource_.get()) {
		dndSource_.reset();
	} else if(&src == clipboardSource_.get()) {
		dndSource_.reset();
	} else {
		dlg_warn("invalid data source object to destroy");
	}
}

bool WaylandAppContext::dispatchDisplay() {
	wakeup_ = false;
	int ret;

	if(wl_display_prepare_read(wlDisplay_) == -1) {
		return wl_display_dispatch_pending(wlDisplay_);
	}

	// try to flush the display until all data is flushed
	while(true) {
		ret = wl_display_flush(wlDisplay_);
		if(ret != -1 || errno != EAGAIN) {
			break;
		}

		// poll until the display could be written again.
		if(pollFds(POLLOUT, -1) == -1) {
			wl_display_cancel_read(wlDisplay_);
			return false;
		}

		// if polling exited since the eventfd was triggered, cancel the
		// wayland read
		if(wakeup_) {
			wl_display_cancel_read(wlDisplay_);
			return true;
		}
	}

	// needed for protocol error to be queried (EPIPE)
	if(ret < 0 && errno != EPIPE) {
		wl_display_cancel_read(wlDisplay_);
		return true;
	}

	// poll for server events (and since this might block for fd callbacks)
	if(pollFds(POLLIN, -1) == -1) {
		wl_display_cancel_read(wlDisplay_);
		return false;
	}

	// if dpypoll stopped due to the eventfd, cancel the wayland read
	if(wakeup_) {
		wl_display_cancel_read(wlDisplay_);
		return true;
	}

	if(wl_display_read_events(wlDisplay_) == -1) {
		return false;
	}

	auto dispatched = wl_display_dispatch_pending(wlDisplay_);
	return dispatched >= 0;
}

int WaylandAppContext::pollFds(short wlDisplayEvents, int timeout) {
	// This function simply polls for all fd callbacks (and the wayland display fd if
	// wlDisplayEvents != 0) with the given timeout and triggers the activated fd callbacks.
	//
	// This has to be done this complex since the callback functions may actually disconnect
	// itself or other connections, i.e. impl_->fdCallbacks may change.
	// We cannot use the fd as id, since there might be multiple callbacks on the same fd.
	// ids holds the connectionID of the associated callback in fds (i.e. the pollfd with the
	// same index)
	//
	// Function has to be re-entrent via the called callbacks (via poll/waitEvents)

	std::vector<nytl::ConnectionID> ids;
	std::vector<pollfd> fds;
	ids.reserve(impl_->fdCallbacks.items.size());
	fds.reserve(impl_->fdCallbacks.items.size() + 1);

	for(auto& fdc : impl_->fdCallbacks.items) {
		fds.push_back({fdc.fd, static_cast<short>(fdc.events), 0u});
		ids.push_back({fdc.clID_});
	}

	// add the wayland display fd to the pollfds
	if(wlDisplayEvents) {
		fds.push_back({wl_display_get_fd(&wlDisplay()), wlDisplayEvents, 0u});
	}

	auto ret = noSigPoll(*fds.data(), fds.size(), timeout);
	if(ret < 0) {
		dlg_info("poll failed: {}", std::strerror(errno));
		return ret;
	}

	// check which fd callbacks have revents, find and trigger them
	// fds.size() > ids.size() always true
	for(auto i = 0u; i < ids.size(); ++i) {
		if(!fds[i].revents) {
			continue;
		}

		auto& items = impl_->fdCallbacks.items;
		auto it = std::find_if(items.begin(), items.end(),
			[&](auto& cb){ return cb.clID_.get() == ids[i].get(); });
		if(it == items.end()) {
			// in this case it was erased by a previous callback
			continue;
		}

		if(!it->callback(fds[i].fd, fds[i].revents)) {
			items.erase(it);
		}
	}

	return ret;
}

void WaylandAppContext::roundtrip() {
	wl_display_roundtrip_queue(&wlDisplay(), wlRoundtripQueue_);
}

void WaylandAppContext::handleRegistryAdd(wl_registry*, uint32_t id, const char* cinterface,
		uint32_t version) {
	// listeners
	// they must be added instantly, otherwise we will miss initial events
	using WAC = WaylandAppContext;
	constexpr static wl_shm_listener shmListener {
		memberCallback<&WAC::handleShmFormat>
	};

	constexpr static wl_seat_listener seatListener {
		memberCallback<&WAC::handleSeatCapabilities>,
		memberCallback<&WAC::handleSeatName>
	};

	constexpr static zxdg_shell_v6_listener xdgShellV6Listener {
		memberCallback<&WAC::handleXdgShellV6Ping>
	};

	// the supported interface versions by ny (for stable protocols)
	// we always select the minimum between version supported by ny and version
	// supported by the compositor
	static constexpr auto compositorVersion = 1u;
	static constexpr auto shellVersion = 1u;
	static constexpr auto shmVersion = 1u;
	static constexpr auto subcompositorVersion = 1u;
	static constexpr auto dataDeviceManagerVersion = 3u;
	static constexpr auto seatVersion = 5u;

	const std::string_view interface = cinterface; // equal comparison using ==
	// debug("ny::WaylandAppContext::handleRegistryAdd: interface ", interface);

	// check for the various supported interfaces/protocols
	if(interface == "wl_compositor" && !impl_->wlCompositor) {
		auto usedVersion = std::min(version, compositorVersion);
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_compositor_interface, usedVersion);
		impl_->wlCompositor = {static_cast<wl_compositor*>(ptr), id};
	} else if(interface == "wl_shell" && !wlShell()) {
		auto usedVersion = std::min(version, shellVersion);
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_shell_interface, usedVersion);
		impl_->wlShell = {static_cast<wl_shell*>(ptr), id};
	} else if(interface == "wl_shm" && !wlShm()) {
		auto usedVersion = std::min(version, shmVersion);
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_shm_interface, usedVersion);
		impl_->wlShm = {static_cast<wl_shm*>(ptr), id};
		wl_shm_add_listener(wlShm(), &shmListener, this);
	} else if(interface == "wl_subcompositor" && !impl_->wlSubcompositor) {
		auto usedVersion = std::min(version, subcompositorVersion);
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_subcompositor_interface, usedVersion);
		impl_->wlSubcompositor = {static_cast<wl_subcompositor*>(ptr), id};
	} else if(interface == "wl_data_device_manager" && !impl_->wlDataManager) {
		auto usedVersion = std::min(version, dataDeviceManagerVersion);
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_data_device_manager_interface,
			usedVersion);
		impl_->wlDataManager = {static_cast<wl_data_device_manager*>(ptr), id};
	} else if(interface == "wl_seat" && !impl_->wlSeat) {
		auto usedVersion = std::min(version, seatVersion);
		auto ptr = wl_registry_bind(&wlRegistry(), id, &wl_seat_interface, usedVersion);
		impl_->wlSeat = {static_cast<wl_seat*>(ptr), id};
		wl_seat_add_listener(wlSeat(), &seatListener, this);
	} else if(interface == "zxdg_shell_v6" && !impl_->xdgShellV6) {
		if(version != 1) return;

		auto ptr = wl_registry_bind(&wlRegistry(), id, &zxdg_shell_v6_interface, 1);
		impl_->xdgShellV6 = {static_cast<zxdg_shell_v6*>(ptr), id};
		zxdg_shell_v6_add_listener(xdgShellV6(), &xdgShellV6Listener, this);
	}
}

void WaylandAppContext::handleRegistryRemove(wl_registry*, uint32_t id) {
	// TODO: stop the application/main loop when removing needed global? at least handle it somehow.
	// TODO: check other globals here?
	// This is currently more like a dummy and not really useful...
	if(id == impl_->wlCompositor.name) {
		wl_compositor_destroy(&wlCompositor());
		impl_->wlCompositor = {};
	}
}

void WaylandAppContext::handleSeatCapabilities(wl_seat*, uint32_t caps) {
	// mouse
	if((caps & WL_SEAT_CAPABILITY_POINTER) && !mouseContext_) {
		mouseContext_ = std::make_unique<WaylandMouseContext>(*this, *wlSeat());
	} else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && mouseContext_) {
		dlg_info("lost wl_pointer");
		mouseContext_.reset();
	}

	// keyboard
	if((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !keyboardContext_) {
		keyboardContext_ = std::make_unique<WaylandKeyboardContext>(*this, *wlSeat());
	} else if(!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && keyboardContext_) {
		dlg_info("lost wl_keyboard");
		keyboardContext_.reset();
	}
}

void WaylandAppContext::handleSeatName(wl_seat*, const char* name) {
	seatName_ = name;
}

void WaylandAppContext::handleShmFormat(wl_shm*, unsigned int format) {
	shmFormats_.push_back(format);
}

void WaylandAppContext::handleXdgShellV6Ping(zxdg_shell_v6*, unsigned int serial) {
	if(!xdgShellV6()) {
		dlg_warn("xdg shell v6 ping but didn't receive global");
		return;
	}

	zxdg_shell_v6_pong(xdgShellV6(), serial);
}

bool WaylandAppContext::shmFormatSupported(unsigned int wlShmFormat) {
	for(auto format : shmFormats_) if(format == wlShmFormat) return true;
	return false;
}

WaylandWindowContext* WaylandAppContext::windowContext(wl_surface& surface) const {
	auto data = wl_surface_get_user_data(&surface);
	return static_cast<WaylandWindowContext*>(data);
}

wl_pointer* WaylandAppContext::wlPointer() const {
	if(!waylandMouseContext()) return nullptr;
	return waylandMouseContext()->wlPointer();
}

wl_keyboard* WaylandAppContext::wlKeyboard() const {
	if(!waylandKeyboardContext()) return nullptr;
	return waylandKeyboardContext()->wlKeyboard();
}

// getters
wl_display& WaylandAppContext::wlDisplay() const { return *wlDisplay_; }
wl_registry& WaylandAppContext::wlRegistry() const { return *wlRegistry_; }
wl_compositor& WaylandAppContext::wlCompositor() const { return *impl_->wlCompositor; }
wl_subcompositor* WaylandAppContext::wlSubcompositor() const{ return impl_->wlSubcompositor; }
wl_shm* WaylandAppContext::wlShm() const { return impl_->wlShm; }
wl_seat* WaylandAppContext::wlSeat() const { return impl_->wlSeat; }
wl_shell* WaylandAppContext::wlShell() const { return impl_->wlShell; }
zxdg_shell_v6* WaylandAppContext::xdgShellV6() const { return impl_->xdgShellV6; }
wl_data_device_manager* WaylandAppContext::wlDataManager() const { return impl_->wlDataManager; }
wl_cursor_theme* WaylandAppContext::wlCursorTheme() const { return wlCursorTheme_; }

} // namespace ny
