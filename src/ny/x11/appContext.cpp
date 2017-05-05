// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/appContext.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/x11/input.hpp>
#include <ny/x11/bufferSurface.hpp>
#include <ny/x11/dataExchange.hpp>

#include <ny/common/unix.hpp>
#include <ny/loopControl.hpp>
#include <ny/log.hpp>
#include <ny/dataExchange.hpp>

#ifdef NY_WithVulkan
	#define VK_USE_PLATFORM_XCB_KHR
	#include <ny/x11/vulkan.hpp>
	#include <vulkan/vulkan.h>
#endif //Vulkan

#ifdef NY_WithGl
	#include <ny/x11/glx.hpp>
#endif //GL

#include <nytl/scope.hpp>

// #include <X11/Xlibint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlib-xcb.h>

// undefine the worst Xlib macros
#undef None
#undef GenericEvent

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#include <cstring>
#include <mutex>
#include <atomic>
#include <queue>

namespace ny {
namespace {

// TODO: reimplement this using eventfd and allow X11AppContext to handle fd callbacks
// should be more efficient and offer more possibilities
// See WaylandAppContexst for details
// See also the common unix AppContext interface concept

/// X11 LoopInterface implementation.
/// Wakes up a blocking xcb_connection by simply sending a client message to
/// a dummy window.
class X11LoopImpl : public ny::LoopInterface {
public:
	xcb_connection_t& xConnection;
	xcb_window_t xDummyWindow {};

	std::atomic<bool> run {true};
	std::queue<std::function<void()>> functions;
	std::mutex mutex;

public:
	X11LoopImpl(LoopControl& control, xcb_connection_t& conn, xcb_window_t win)
		:  LoopInterface(control), xConnection(conn), xDummyWindow(win) {}

	bool stop() override
	{
		run.store(false);
		wakeup();
		return true;
	}

	bool call(std::function<void()> function) override
	{
		if(!function) return false;

		{
			std::lock_guard<std::mutex> lock(mutex);
			functions.push(std::move(function));
		}

		wakeup();
		return true;
	}

	void wakeup()
	{
		xcb_client_message_event_t dummyEvent {};
		dummyEvent.window = xDummyWindow;
		dummyEvent.response_type = XCB_CLIENT_MESSAGE;
		dummyEvent.format = 32;

		auto eventData = reinterpret_cast<const char*>(&dummyEvent);
		xcb_send_event(&xConnection, 0, xDummyWindow, 0, eventData);
		xcb_flush(&xConnection);
	}

	std::function<void()> popFunction()
	{
		std::lock_guard<std::mutex> lock(mutex);
		if(functions.empty()) return {};
		auto ret = std::move(functions.front());
		functions.pop();
		return ret;
	}
};

} // anonymous util namespace

struct X11AppContext::Impl {
	x11::EwmhConnection ewmhConnection;
	x11::Atoms atoms;
	X11ErrorCategory errorCategory;
	X11DataManager dataManager;

#ifdef NY_WithGl
	GlxSetup glxSetup;
	bool glxFailed;
#endif //GL
};

// AppContext
X11AppContext::X11AppContext()
{
	// TODO, make this optional, may be needed by some applications
	// Something like a threadsafe flags in X11AppContextSettings would make sense
	// XInitThreads();

	impl_ = std::make_unique<Impl>();

	xDisplay_ = ::XOpenDisplay(nullptr);
	if(!xDisplay_)
		throw std::runtime_error("ny::X11AppContext: could not connect to X Server");

	xDefaultScreenNumber_ = ::XDefaultScreen(xDisplay_);

	xConnection_ = ::XGetXCBConnection(xDisplay_);
	if(!xConnection_ || xcb_connection_has_error(xConnection_))
		throw std::runtime_error("ny::X11AppContext: unable to get xcb connection");

	impl_->errorCategory = {*xDisplay_, *xConnection_};
	auto ewmhCookie = xcb_ewmh_init_atoms(&xConnection(), &ewmhConnection());

	// query server information
	auto iter = xcb_setup_roots_iterator(xcb_get_setup(&xConnection()));
	for(auto i = 0; iter.rem; ++i, xcb_screen_next(&iter)) {
		if(i == xDefaultScreenNumber_) {
			xDefaultScreen_ = iter.data;
			break;
		}
	}

	// This must be called because xcb is used to access the event queue
	::XSetEventQueueOwner(xDisplay_, XCBOwnsEventQueue);

	// Generate an x dummy window that can e.g. be used for selections
	// This window remains invisible, i.e. it is not begin mapped
	xDummyWindow_ = xcb_generate_id(xConnection_);
	auto cookie = xcb_create_window_checked(xConnection_, XCB_COPY_FROM_PARENT, xDummyWindow_,
		xDefaultScreen_->root, 0, 0, 50, 50, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
		XCB_COPY_FROM_PARENT, 0, nullptr);
	errorCategory().checkThrow(cookie, "ny::X11AppContext: create_window for dummy window failed");

	// Load all default required atoms
	auto& atoms = impl_->atoms;
	struct {
		xcb_atom_t& atom;
		const char* name;
	} atomNames[] = {
		{atoms.xdndEnter, "XdndEnter"},
		{atoms.xdndPosition, "XdndPosition"},
		{atoms.xdndStatus, "XdndStatus"},
		{atoms.xdndTypeList, "XdndTypeList"},
		{atoms.xdndActionCopy, "XdndActionCopy"},
		{atoms.xdndActionMove, "XdndActionMove"},
		{atoms.xdndActionAsk, "XdndActionAsk"},
		{atoms.xdndActionLink, "XdndActionLink"},
		{atoms.xdndDrop, "XdndDrop"},
		{atoms.xdndLeave, "XdndLeave"},
		{atoms.xdndFinished, "XdndFinished"},
		{atoms.xdndSelection, "XdndSelection"},
		{atoms.xdndProxy, "XdndProxy"},
		{atoms.xdndAware, "XdndAware"},

		{atoms.clipboard, "CLIPBOARD"},
		{atoms.targets, "TARGETS"},
		{atoms.text, "TEXT"},
		{atoms.utf8string, "UTF8_STRING"},
		{atoms.fileName, "FILE_NAME"},

		{atoms.wmDeleteWindow, "WM_DELETE_WINDOW"},
		{atoms.motifWmHints, "_MOTIF_WM_HINTS"},

		{atoms.mime.textPlain, "text/plain"},
		{atoms.mime.textPlainUtf8, "text/plain;charset=utf8"},
		{atoms.mime.textUriList, "text/uri-list"},

		{atoms.mime.imageData, "image/x-ny-data"},
		{atoms.mime.raw, "application/octet-stream"}
	};

	auto length = sizeof(atomNames) / sizeof(atomNames[0]);

	std::vector<xcb_intern_atom_cookie_t> atomCookies;
	atomCookies.reserve(length);

	for(auto& name : atomNames)
		atomCookies.push_back(xcb_intern_atom(xConnection_, 0, std::strlen(name.name), name.name));

	for(auto i = 0u; i < atomCookies.size(); ++i) {
		xcb_generic_error_t* error {};
		auto reply = xcb_intern_atom_reply(xConnection_, atomCookies[i], &error);
		if(reply) {
			atomNames[i].atom = reply->atom;
			free(reply);
			continue;
		} else if(error) {
			auto msg = x11::errorMessage(xDisplay(), error->error_code);
			warning("ny::X11AppContext: Failed to load atom ", atomNames[i].name, ": ", msg);
			free(error);
		}
	}

	// ewmh atoms
	xcb_ewmh_init_atoms_replies(&ewmhConnection(), ewmhCookie, nullptr);

	// input
	keyboardContext_ = std::make_unique<X11KeyboardContext>(*this);
	mouseContext_ = std::make_unique<X11MouseContext>(*this);

	// data manager
	impl_->dataManager = {*this};
}

X11AppContext::~X11AppContext()
{
	if(xDisplay_) ::XFlush(&xDisplay());

	xcb_ewmh_connection_wipe(&ewmhConnection());
	impl_.reset();

	if(xDummyWindow_) xcb_destroy_window(xConnection_, xDummyWindow_);
	if(xDisplay_) ::XCloseDisplay(&xDisplay());

	xDisplay_ = nullptr;
	xConnection_ = nullptr;
	xDefaultScreen_ = nullptr;
}

WindowContextPtr X11AppContext::createWindowContext(const WindowSettings& settings)
{
	X11WindowSettings x11Settings;
	const auto* ws = dynamic_cast<const X11WindowSettings*>(&settings);

	if(ws) x11Settings = *ws;
	else x11Settings.WindowSettings::operator=(settings);

	// type
	if(settings.surface == SurfaceType::vulkan) {
		#ifdef NY_WithVulkan
			return std::make_unique<X11VulkanWindowContext>(*this, x11Settings);
		#else
			throw std::logic_error("ny::X11AppContext::createWindowContext: "
				"ny built without vulkan support, cannot create the requested vulkan surface");
		#endif
	} else if(settings.surface == SurfaceType::gl) {
		#ifdef NY_WithGl
			static constexpr auto glxFailed = "ny::X11AppContext::createWindowContext: "
				"failed to init glx, cannot create the requested gl surface";

			if(!glxSetup()) throw std::runtime_error(glxFailed);
			return std::make_unique<GlxWindowContext>(*this, *glxSetup(), x11Settings);
		#else
			throw std::logic_error("ny::X11AppContext::createWindowContext: "
				"ny built without gl/glx support, cannot create the requested gl surface");
		#endif
	} else if(settings.surface == SurfaceType::buffer) {
		return std::make_unique<X11BufferWindowContext>(*this, x11Settings);
	}

	return std::make_unique<X11WindowContext>(*this, x11Settings);
}

MouseContext* X11AppContext::mouseContext()
{
	return mouseContext_.get();
}

KeyboardContext* X11AppContext::keyboardContext()
{
	return keyboardContext_.get();
}

bool X11AppContext::dispatchEvents()
{
	if(!checkErrorWarn()) return false;

	xcb_flush(&xConnection());
	while(auto event = xcb_poll_for_event(xConnection_)) {
		processEvent(static_cast<const x11::GenericEvent&>(*event));
		free(event);
		xcb_flush(&xConnection());
	}

	return checkErrorWarn();
}

bool X11AppContext::dispatchLoop(LoopControl& control)
{
	X11LoopImpl loopImpl(control, xConnection(), xDummyWindow());

	while(loopImpl.run.load()) {
		while(auto func = loopImpl.popFunction()) func();

		xcb_generic_event_t* event = xcb_wait_for_event(xConnection_);
		if(!event && !checkErrorWarn()) return false;

		processEvent(static_cast<const x11::GenericEvent&>(*event));
		free(event);
		xcb_flush(&xConnection());
	}

	return true;
}

bool X11AppContext::clipboard(std::unique_ptr<DataSource>&& dataSource)
{
	return impl_->dataManager.clipboard(std::move(dataSource));
}

DataOffer* X11AppContext::clipboard()
{
	return impl_->dataManager.clipboard();
}

bool X11AppContext::startDragDrop(std::unique_ptr<DataSource>&& dataSource)
{
	return impl_->dataManager.startDragDrop(std::move(dataSource));
}

std::vector<const char*> X11AppContext::vulkanExtensions() const
{
	#ifdef NY_WithVulkan
		return {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XCB_SURFACE_EXTENSION_NAME};
	#else
		return {};
	#endif
}

GlSetup* X11AppContext::glSetup() const
{
	#ifdef NY_WithGl
		return glxSetup();
	#else
		return nullptr;
	#endif
}

GlxSetup* X11AppContext::glxSetup() const
{
	#ifdef NY_WithGl
		if(impl_->glxFailed) return nullptr;

		if(!impl_->glxSetup.valid()) {
			try {
				impl_->glxSetup = {*this};
			} catch(const std::exception& error) {
				warning("ny::X11AppContext::glxSetup: initialization failed: ", error.what());
				impl_->glxFailed = true;
				impl_->glxSetup = {};
				return nullptr;
			}
		}

		return &impl_->glxSetup;

	#else
		return nullptr;

	#endif
}

void X11AppContext::registerContext(xcb_window_t w, X11WindowContext& c)
{
	contexts_[w] = &c;
}

void X11AppContext::unregisterContext(xcb_window_t w)
{
	contexts_.erase(w);
}

X11WindowContext* X11AppContext::windowContext(xcb_window_t win)
{
	if(contexts_.find(win) != contexts_.end())
		return contexts_[win];

	return nullptr;
}

bool X11AppContext::checkErrorWarn()
{
	auto err = xcb_connection_has_error(xConnection_);
	if(err) {
		error("ny::X11AppContext: xcb_connection has critical error ", err);
		return false;
	}

	return true;
}

xcb_atom_t X11AppContext::atom(const std::string& name)
{
	auto it = additionalAtoms_.find(name);
	if(it == additionalAtoms_.end()) {
		auto cookie = xcb_intern_atom(xConnection_, 0, name.size(), name.c_str());
		xcb_generic_error_t* error;
		auto reply = xcb_intern_atom_reply(xConnection_, cookie, &error);
		if(error) {
			auto msg = x11::errorMessage(xDisplay(), error->error_code);
			warning("ny::X11AppContext::atom: failed to retrieve atom ", name, ": ", msg);
			free(error);
			return 0u;
		}

		it = additionalAtoms_.insert({name, reply->atom}).first;
		free(reply);
	}

	return it->second;
}

void X11AppContext::bell()
{
	// TODO: rather random value here. accept (defaulted) parameter?
	xcb_bell(xConnection_, 100);
}

void X11AppContext::processEvent(const x11::GenericEvent& ev)
{
	X11EventData eventData {ev};

	auto responseType = ev.response_type & ~0x80;
	switch(responseType) {
		case XCB_EXPOSE: {
			auto& expose = reinterpret_cast<const xcb_expose_event_t&>(ev);
			auto wc = windowContext(expose.window);
			if(expose.count == 0 && wc) {
				DrawEvent de;
				de.eventData = &eventData;
				wc->listener().draw(de);
			}
			break;
		}

		case XCB_MAP_NOTIFY: {
			auto& map = reinterpret_cast<const xcb_map_notify_event_t&>(ev);
			auto wc = windowContext(map.event);
			if(wc) {
				DrawEvent de;
				de.eventData = &eventData;
				wc->listener().draw(de);
			}
			break;
		}

		case XCB_REPARENT_NOTIFY: {
			auto& reparent = reinterpret_cast<const xcb_reparent_notify_event_t&>(ev);
			auto wc = windowContext(reparent.window);
			if(wc) wc->reparentEvent();
			break;
		}

		case XCB_CONFIGURE_NOTIFY: {
			auto& configure = reinterpret_cast<const xcb_configure_notify_event_t&>(ev);

			auto nsize = nytl::Vec2ui{configure.width, configure.height};
			// auto npos = nytl::Vec2i(configure.x, configure.y);

			auto wc = windowContext(configure.window);
			if(wc) {
				SizeEvent se;
				se.eventData = &eventData;
				se.size = nsize;
				wc->listener().resize(se);
				// wc->listener().position({{&eventData}, npos});
			}

			break;
		}

		case XCB_CLIENT_MESSAGE: {
			auto& client = reinterpret_cast<const xcb_client_message_event_t&>(ev);
			auto protocol = static_cast<unsigned int>(client.data.data32[0]);

			auto wc = windowContext(client.window);
			if(protocol == atoms().wmDeleteWindow && wc) {
				CloseEvent ce;
				ce.eventData = &eventData;
				wc->listener().close(ce);
			}

			break;
		}

	case 0u: {
			int code = reinterpret_cast<const xcb_generic_error_t&>(ev).error_code;
			auto errorMsg = x11::errorMessage(xDisplay(), code);
			warning("ny::X11AppContext::processEvent: retrieved error code ", code, ": ", errorMsg);

			break;
		}

		default: break;
	}

	if(impl_->dataManager.processEvent(ev)) return;
	if(keyboardContext_->processEvent(ev)) return;
	if(mouseContext_->processEvent(ev)) return;

	#undef EventHandlerEvent
}

x11::EwmhConnection& X11AppContext::ewmhConnection() const { return impl_->ewmhConnection; }
X11ErrorCategory& X11AppContext::errorCategory() const { return impl_->errorCategory; }
const x11::Atoms& X11AppContext::atoms() const { return impl_->atoms; }
X11DataManager& X11AppContext::dataManager() const { return impl_->dataManager; }

}
