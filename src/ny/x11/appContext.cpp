// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/appContext.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/x11/input.hpp>
#include <ny/x11/bufferSurface.hpp>

#include <ny/common/unix.hpp>
#include <ny/loopControl.hpp>
#include <ny/log.hpp>
#include <ny/events.hpp>
#include <ny/data.hpp>

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

//undefine the most cancerous Xlib macros
#undef None
#undef GenericEvent

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#include <cstring>
#include <mutex>
#include <queue>

namespace ny
{

namespace
{

//TODO: reimplement this using eventfd and allow X11AppContext to handle fd callbacks
///X11 LoopInterface implementation.
///Wakes up a blocking xcb_connection by simply sending a client message to
///a dummy window.
class X11LoopImpl : public ny::LoopInterfaceGuard
{
public:
	xcb_connection_t& xConnection;
	xcb_window_t xDummyWindow {};

	std::atomic<bool> run {true};
	std::queue<std::function<void()>> functions;
	std::mutex mutex {};

public:
	X11LoopImpl(LoopControl& control, xcb_connection_t& conn, xcb_window_t win)
		:  LoopInterfaceGuard(control), xConnection(conn), xDummyWindow(win) {}

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

}

struct X11AppContext::Impl
{
	x11::EwmhConnection ewmhConnection;
	x11::Atoms atoms;
	X11ErrorCategory errorCategory;

#ifdef NY_WithGl
	GlxSetup glxSetup;
	bool glxFailed;
#endif //GL
};

//appContext
X11AppContext::X11AppContext()
{
    //XInitThreads(); //todo, make this optional
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

	//query information
	auto iter = xcb_setup_roots_iterator(xcb_get_setup(&xConnection()));
	for(auto i = 0; iter.rem; ++i, xcb_screen_next(&iter))
	{
	    if(i == xDefaultScreenNumber_)
		{
			xDefaultScreen_ = iter.data;
			break;
	    }
	}

	//This must be called because xcb is used to access the event queue
    ::XSetEventQueueOwner(xDisplay_, XCBOwnsEventQueue);

	//Generate an x dummy window that can e.g. be used for selections
	//This window remains invisible, i.e. it is not begin mapped
	xDummyWindow_ = xcb_generate_id(xConnection_);
    auto cookie = xcb_create_window_checked(xConnection_, XCB_COPY_FROM_PARENT, xDummyWindow_,
		xDefaultScreen_->root, 0, 0, 50, 50, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
		XCB_COPY_FROM_PARENT, 0, nullptr);
	errorCategory().checkThrow(cookie, "ny::X11AppContext: create_window for dummy window failed");

	//Load all default required atoms
	auto& atoms = impl_->atoms;
	struct
	{
		xcb_atom_t& atom;
		const char* name;
	} atomNames[] =
	{
		{atoms.xdndEnter, "XdndEnter"},
		{atoms.xdndPosition, "XdndPosition"},
		{atoms.xdndStatus, "XdndStatus"},
		{atoms.xdndTypeList, "XdndTypeList"},
		{atoms.xdndActionCopy, "XdndActionCopy"},
		{atoms.xdndActionMove, "XdndActionMove"},
		{atoms.xdndActionAsk, "XdndActionAsk"},
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

		{atoms.mime.imageJpeg, "image/jpeg"},
		{atoms.mime.imageGif, "image/gif"},
		{atoms.mime.imagePng, "image/png"},
		{atoms.mime.imageBmp, "image/bmp"},

		{atoms.mime.imageData, "image/x-ny-data"},
		{atoms.mime.timePoint, "x-application/ny-time-point"},
		{atoms.mime.timeDuration, "x-application/ny-time-duration"},
		{atoms.mime.raw, "x-application/ny-raw-buffer"},
	};

	auto length = sizeof(atomNames) / sizeof(atomNames[0]);

	std::vector<xcb_intern_atom_cookie_t> atomCookies;
	atomCookies.reserve(length);

	for(auto& name : atomNames)
		atomCookies.push_back(xcb_intern_atom(xConnection_, 0, std::strlen(name.name), name.name));

	for(auto i = 0u; i < atomCookies.size(); ++i)
	{
		xcb_generic_error_t* error {};
		auto reply = xcb_intern_atom_reply(xConnection_, atomCookies[i], &error);
		if(reply)
		{
			atomNames[i].atom = reply->atom;
			free(reply);
			continue;
		}
		else if(error)
		{
			auto msg = x11::errorMessage(xDisplay(), error->error_code);
			free(error);
			warning("ny::X11AppContext: Failed to load atom ", atomNames[i].name, ": ", msg);
		}
	}

	//ewmh
	xcb_ewmh_init_atoms_replies(&ewmhConnection(), ewmhCookie, nullptr);

	//input
	keyboardContext_ = std::make_unique<X11KeyboardContext>(*this);
	mouseContext_ = std::make_unique<X11MouseContext>(*this);
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

EventHandler* X11AppContext::eventHandler(xcb_window_t w)
{
    auto* wc = windowContext(w);
    return wc ? wc->eventHandler() : nullptr;
}

WindowContextPtr X11AppContext::createWindowContext(const WindowSettings& settings)
{
    X11WindowSettings x11Settings;
    const auto* ws = dynamic_cast<const X11WindowSettings*>(&settings);

    if(ws) x11Settings = *ws;
    else x11Settings.WindowSettings::operator=(settings);

	//type
	if(settings.surface == SurfaceType::vulkan)
	{
		#ifdef NY_WithVulkan
			return std::make_unique<X11VulkanWindowContext>(*this, x11Settings);
		#else
			static constexpr auto noVulkan = "ny::X11AppContext::createWindowContext: "
				"ny built without vulkan support, cannot create the requested vulkan surface";

			throw std::logic_error(noVulkan);
		#endif
	}
	else if(settings.surface == SurfaceType::gl)
	{
		#ifdef NY_WithGl
			static constexpr auto glxFailed = "ny::X11AppContext::createWindowContext: "
				"failed to init glx, cannot create the requested gl surface";

			if(!glxSetup()) throw std::runtime_error(glxFailed);
			return std::make_unique<GlxWindowContext>(*this, *glxSetup(), x11Settings);
		#else
			static constexpr auto noGlx = "ny::X11AppContext::createWindowContext: "
				"ny built without gl/glx support, cannot create the requested gl surface";

			throw std::logic_error(noGlx);
		#endif
	}
	else if(settings.surface == SurfaceType::buffer)
	{
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
	while(auto event = xcb_poll_for_event(xConnection_))
	{
		processEvent(static_cast<const x11::GenericEvent&>(*event));
		free(event);
		xcb_flush(&xConnection());
	}

	return checkErrorWarn();
}

bool X11AppContext::dispatchLoop(LoopControl& control)
{
	X11LoopImpl loopImpl(control, xConnection(), xDummyWindow());

	while(loopImpl.run.load())
	{
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
	nytl::unused(dataSource);
	return false;
}

DataOffer* X11AppContext::clipboard()
{
	return nullptr;
}

bool X11AppContext::startDragDrop(std::unique_ptr<DataSource>&& dataSource)
{
	nytl::unused(dataSource);
	return false;
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

		if(!impl_->glxSetup.valid())
		{
			try { impl_->glxSetup = {*this}; }
			catch(const std::exception& error)
			{
				warning("ny::X11Setup::glxSetup: initialization failed: ", error.what());
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
	if(err)
	{
		error("ny::X11AppContext: xcb_connection has critical error ", err);
		return false;
	}

	return true;
}

xcb_atom_t X11AppContext::atom(const std::string& name)
{
	auto it = additionalAtoms_.find(name);
	if(it == additionalAtoms_.end())
	{
		auto cookie = xcb_intern_atom(xConnection_, 0, name.size(), name.c_str());
		xcb_generic_error_t* error;
		auto reply = xcb_intern_atom_reply(xConnection_, cookie, &error);
		if(error)
		{
			auto msg = x11::errorMessage(xDisplay(), error->error_code);
			warning("ny::X11AppContext::atom: failed to retrieve ", name, ": ", msg);
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
	xcb_bell(xConnection_, 100);
}

x11::EwmhConnection& X11AppContext::ewmhConnection() const { return impl_->ewmhConnection; }
const X11ErrorCategory& X11AppContext::errorCategory() const { return impl_->errorCategory; }
const x11::Atoms& X11AppContext::atoms() const { return impl_->atoms; }

void X11AppContext::processEvent(const x11::GenericEvent& ev)
{
	//macro for easier event creation for registered EventHandler
	#define EventHandlerEvent(T, W) \
		auto handler = eventHandler(W); \
		if(!handler) return; \
		auto event = T(handler);

	auto dispatch = [&](Event& event){
		if(event.handler) event.handler->handleEvent(event);
	};

	auto responseType = ev.response_type & ~0x80;
    switch(responseType)
    {

    case XCB_MOTION_NOTIFY:
    {
		auto& motion = reinterpret_cast<const xcb_motion_notify_event_t&>(ev);
		auto pos = nytl::Vec2i(motion.event_x, motion.event_y);
		mouseContext_->move(pos);

		EventHandlerEvent(MouseMoveEvent, motion.event);
        event.position = pos;
        event.screenPosition = nytl::Vec2i(motion.root_x, motion.root_y);

		dispatch(event);
		break;
    }

    case XCB_EXPOSE:
    {
		auto& expose = reinterpret_cast<const xcb_expose_event_t&>(ev);
        if(expose.count == 0)
		{
			EventHandlerEvent(DrawEvent, expose.window);
			dispatch(event);
		}

		break;
    }

    case XCB_MAP_NOTIFY:
    {
		auto& map = reinterpret_cast<const xcb_map_notify_event_t&>(ev);
		EventHandlerEvent(DrawEvent, map.window);
		dispatch(event);
		break;
    }

    case XCB_BUTTON_PRESS:
    {
		auto& button = reinterpret_cast<const xcb_button_press_event_t&>(ev);

		int scroll = 0;
		if(button.detail == 4) scroll = 1;
		else if(button.detail == 5) scroll = -1;

		if(scroll)
		{
			EventHandlerEvent(MouseWheelEvent, button.event);
			event.data = std::make_unique<X11EventData>(ev);
			event.value = scroll;

			mouseContext_->onWheel(*mouseContext_, scroll);
			dispatch(event);

			break;
		}

		auto b = x11ToButton(button.detail);
		mouseContext_->mouseButton(b, true);

		EventHandlerEvent(MouseButtonEvent, button.event);
		event.data = std::make_unique<X11EventData>(ev);
        event.button = b;
        event.position = nytl::Vec2i(button.event_x, button.event_y);
		event.pressed = true;

		dispatch(event);
		break;
    }

    case XCB_BUTTON_RELEASE:
    {
		auto& button = reinterpret_cast<const xcb_button_release_event_t&>(ev);
		if(button.detail == 4 || button.detail == 5) break;

		auto b = x11ToButton(button.detail);
		mouseContext_->mouseButton(b, false);

		EventHandlerEvent(MouseButtonEvent, button.event);
		event.data = std::make_unique<X11EventData>(ev);
        event.button = b;
        event.position = nytl::Vec2i(button.event_x, button.event_y);
		event.pressed = false;

		dispatch(event);
		break;
    }

    case XCB_ENTER_NOTIFY:
    {
		auto& enter = reinterpret_cast<const xcb_enter_notify_event_t&>(ev);
		auto wc = windowContext(enter.event);
		mouseContext_->over(wc);

		EventHandlerEvent(MouseCrossEvent, enter.event);
        event.position = nytl::Vec2i(enter.event_x, enter.event_y);
		event.entered = true;
		dispatch(event);

		break;
    }

    case XCB_LEAVE_NOTIFY:
    {
		auto& leave = reinterpret_cast<const xcb_enter_notify_event_t&>(ev);
		auto wc = windowContext(leave.event);
		if(mouseContext_->over() == wc) mouseContext_->over(nullptr);

		EventHandlerEvent(MouseCrossEvent, leave.event);
        event.position = nytl::Vec2i(leave.event_x, leave.event_y);
		event.entered = false;
		dispatch(event);

		break;
    }

    case XCB_FOCUS_IN:
    {
		auto& focus = reinterpret_cast<const xcb_focus_in_event_t&>(ev);
		auto wc = windowContext(focus.event);
		keyboardContext_->focus(wc);

		EventHandlerEvent(FocusEvent, focus.event);
		event.focus = true;
		dispatch(event);

		break;
    }

    case XCB_FOCUS_OUT:
    {
		auto& focus = reinterpret_cast<const xcb_focus_in_event_t&>(ev);
		auto wc = windowContext(focus.event);
		if(keyboardContext_->focus() == wc)keyboardContext_->focus(nullptr);

		EventHandlerEvent(FocusEvent, focus.event);
		event.focus = false;
		dispatch(event);

		break;
    }

    case XCB_KEY_PRESS:
    {
		auto& key = reinterpret_cast<const xcb_key_press_event_t&>(ev);

		EventHandlerEvent(KeyEvent, key.event);
		event.pressed = true;
		if(!keyboardContext_->keyEvent(key.detail, event)) bell();
		dispatch(event);

		break;
    }

    case XCB_KEY_RELEASE:
    {
		auto& key = reinterpret_cast<const xcb_key_press_event_t&>(ev);

		EventHandlerEvent(KeyEvent, key.event);
		event.pressed = false;
		if(!keyboardContext_->keyEvent(key.detail, event)) bell();
		dispatch(event);

		break;
    }

	case XCB_REPARENT_NOTIFY:
	{
		auto& reparent = reinterpret_cast<const xcb_reparent_notify_event_t&>(ev);
		auto wc = windowContext(reparent.window);
		if(wc) wc->reparentEvent();

		break;
	}

    case XCB_CONFIGURE_NOTIFY:
	{
		auto& configure = reinterpret_cast<const xcb_configure_notify_event_t&>(ev);

        //todo: something about window state
        auto nsize = nytl::Vec2ui(configure.width, configure.height);
        // auto npos = nytl::Vec2i(configure.x, configure.y); //positionEvent

		auto wc = windowContext(configure.window);
		if(wc) wc->sizeEvent(nsize);

        if(!eventHandler(configure.window)) break;

		/* TODO XXX !important
        if(any(windowContext(configure.window)->window().size() != nsize)) //sizeEvent
		{
			EventHandlerEvent(SizeEvent, configure.window);
			event->size = nsize;
			event->change = 0;
			dispatcher.dispatch(std::move(event));

			auto wc = windowContext(configure.window);
			if(!wc) return true;
			auto wevent = std::make_unique<SizeEvent>(wc);
			wevent->size = nsize;
			wevent->change = 0;
			dispatcher.dispatch(std::move(wevent));
		}

        if(any(windowContext(configure.window)->window().position() != npos))
		{
			EventHandlerEvent(PositionEvent, configure.window);
			event->position = npos;
			event->change = 0;
			dispatcher.dispatch(std::move(event));
		}
		*/

		break;
    }

    case XCB_CLIENT_MESSAGE:
    {
		auto& client = reinterpret_cast<const xcb_client_message_event_t&>(ev);
		auto protocol = static_cast<unsigned int>(client.data.data32[0]);
        if(protocol == atoms().wmDeleteWindow)
        {
			EventHandlerEvent(CloseEvent, client.window);
			dispatch(event);
        }

		break;
	}

	case 0u:
	{
		//an error occurred!
		int code = reinterpret_cast<const xcb_generic_error_t&>(ev).error_code;
		auto errorMsg = x11::errorMessage(xDisplay(), code);
		warning("ny::X11AppContext::processEvent: retrieved error code ", code, ", ", errorMsg);

		break;
	}

	default:
	{
		//check for xkb event
		if(ev.response_type == keyboardContext_->xkbEventType())
			keyboardContext_->processXkbEvent(ev);

		// May be needed for gl to work correctly... (TODO: test)
		// XLockDisplay(xDisplay_);
	    // auto proc = XESetWireToEvent(xDisplay_, ev.response_type & ~0x80, nullptr);
	    // if(proc)
		// {
	    //     XESetWireToEvent(xDisplay_, ev.response_type & ~0x80, proc);
	    //     XEvent dummy;
	    //     ev.sequence = LastKnownRequestProcessed(xDisplay_);
	    //     if(proc(xDisplay_, &dummy, (xEvent*) &ev)) //not handled
		// 	{
		// 		//TODO
		// 	}
	    // }
		//
		// XUnlockDisplay(xDisplay_);
	}

	}

	#undef EventHandlerEvent
}

}
