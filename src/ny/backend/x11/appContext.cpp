#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/x11/util.hpp>
#include <ny/backend/x11/internal.hpp>
#include <ny/base/loopControl.hpp>
#include <ny/base/log.hpp>
#include <ny/app/app.hpp>
#include <ny/window/window.hpp>
#include <ny/window/events.hpp>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb_ewmh.h>
#include <cstring>


namespace ny
{

//LoopControlImpl
class X11AppContext::LoopControlImpl : public ny::LoopControlImpl
{
public:
	std::atomic<bool>& run;
	xcb_connection_t* xConnection;
	xcb_window_t xDummyWindow;

	LoopControlImpl(std::atomic<bool>& xrun, xcb_connection_t* conn, xcb_window_t win)
		: run(xrun), xConnection(conn), xDummyWindow(win) {}

	virtual void stop() override
	{
		if(!run.load()) return;

		//send dummy event to interrupt loop
		xcb_client_message_event_t dummyEvent {};
		dummyEvent.window = xDummyWindow;
		dummyEvent.type = XCB_CLIENT_MESSAGE;
		dummyEvent.format = 32;

		xcb_send_event(xConnection, 0, xDummyWindow, 0, reinterpret_cast<char*>(&dummyEvent));
		xcb_flush(xConnection);
	}
};

//appContext
X11AppContext::X11AppContext()
{
    //XInitThreads(); //todo, make this optional

	//xDisplay
    xDisplay_ = XOpenDisplay(nullptr);
    if(!xDisplay_)
    {
        throw std::runtime_error("ny::x11AC: could not connect to X Server");
    }

    xDefaultScreenNumber_ = DefaultScreen(xDisplay_);

	//xcb_connection
 	xConnection_ = XGetXCBConnection(xDisplay_);
    if(!xConnection_)
    {
		throw std::runtime_error("ny::x11AC: unable to get xcb connection");
    }

	//ewmh connection
	ewmhConnection_.reset(new dummy_xcb_ewmh_connection_t());
	auto ewmhCookie = xcb_ewmh_init_atoms(xConnection_, ewmhConnection());

	//query screen
	auto iter = xcb_setup_roots_iterator(xcb_get_setup(xConnection_));
	for(std::size_t i(0); iter.rem; ++i, xcb_screen_next(&iter))
    if(i == std::size_t(xDefaultScreenNumber_))
	{
		xDefaultScreen_ = iter.data;
		break;
    }

	//events are queried with xcb
    XSetEventQueueOwner(xDisplay_, XCBOwnsEventQueue);

    //selection events will be sent to this window -> they need no window argument
    //does not need to be mapped
	xDummyWindow_ = xcb_generate_id(xConnection_);
    xcb_create_window(xConnection_, XCB_COPY_FROM_PARENT, xDummyWindow_, xDefaultScreen_->root,
		0, 0, 50, 50, 10, XCB_WINDOW_CLASS_INPUT_ONLY, xDefaultScreen_->root_visual, 0, nullptr);

	//atoms
    std::vector<std::string> names = 
	{
        "XdndEnter",
        "XdndPosition",
        "XdndStatus",
        "XdndTypeList",
        "XdndActionCopy",
        "XdndDrop",
        "XdndLeave",
        "XdndFinished",
        "XdndSelection",
        "XdndProxy",
        "XdndAware",
        "PRIMARY",
        "CLIPBOARD",
        "TARGETS",
        "Text",
        "CARDINAL"
    };

	std::vector<xcb_intern_atom_cookie_t> atomCookies;
	atomCookies.reserve(names.size());

	for(auto& name : names)
	{
		atomCookies.push_back(xcb_intern_atom(xConnection_, 0, name.size(), name.c_str()));
	}

	for(std::size_t i(0); i < names.size(); ++i)
	{
		auto reply = xcb_intern_atom_reply(xConnection_, atomCookies[i], 0);
		if(reply)
		{
			atoms_[names[i]] = reply->atom;
		}
		else
		{
			sendWarning("ny::X11AC: Failed to load atom ", names[i]);
		}
	}

	//ewmh
	xcb_ewmh_init_atoms_replies(ewmhConnection(), ewmhCookie, nullptr);
}

X11AppContext::~X11AppContext()
{
	if(xDummyWindow_)
	{
		xcb_destroy_window(xConnection_, xDummyWindow_);
	}

    if(xDisplay_)
	{
		XFlush(xDisplay_);
		XCloseDisplay(xDisplay_);
	}

	xDisplay_ = nullptr;
	xConnection_ = nullptr;
	xDefaultScreen_ = nullptr;

	atoms_.clear();
	contexts_.clear();
}

EventHandler* X11AppContext::eventHandler(xcb_window_t w)
{
    auto* wc = windowContext(w);
    return wc ? wc->eventHandler() : nullptr;
}

bool X11AppContext::processEvent(xcb_generic_event_t& ev, EventDispatcher& dispatcher)
{
	#define EventHandlerEvent(T, W) \
		auto handler = eventHandler(W); \
		if(!handler) return 1; \
		auto event = std::make_unique<T>(handler);

	auto responseType = ev.response_type & ~0x80;
    switch(responseType)
    {
    case XCB_MOTION_NOTIFY:
    {
		auto& motion = reinterpret_cast<xcb_motion_notify_event_t&>(ev);
		EventHandlerEvent(MouseMoveEvent, motion.event);
        event->position = Vec2i(motion.event_x, motion.event_y);
        event->screenPosition = Vec2i(motion.root_x, motion.root_y);

		dispatcher.dispatch(std::move(event));
        return 1;
    }

    case XCB_EXPOSE:
    {
		auto& expose = reinterpret_cast<xcb_expose_event_t&>(ev);
        if(expose.count == 0)
		{
			EventHandlerEvent(DrawEvent, expose.window);
			dispatcher.dispatch(std::move(event));
		}
        return 1;
    }
    case XCB_MAP_NOTIFY:
    {
		auto& map = reinterpret_cast<xcb_map_notify_event_t&>(ev);
		EventHandlerEvent(DrawEvent, map.window);
		dispatcher.dispatch(std::move(event));
        return 1;
    }

    case XCB_BUTTON_PRESS:
    {
		auto& button = reinterpret_cast<xcb_button_press_event_t&>(ev);
		EventHandlerEvent(MouseButtonEvent, button.event);
		event->data = std::make_unique<X11EventData>(ev);
        event->button = x11ToButton(button.detail);
        event->position = Vec2i(button.event_x, button.event_y);
		event->pressed = 1;

		dispatcher.dispatch(std::move(event));
        return 1;
    }

    case XCB_BUTTON_RELEASE:
    {
		auto& button = reinterpret_cast<xcb_button_release_event_t&>(ev);
		EventHandlerEvent(MouseButtonEvent, button.event);
		event->data = std::make_unique<X11EventData>(ev);
        event->button = x11ToButton(button.detail);
        event->position = Vec2i(button.event_x, button.event_y);
		event->pressed = 0;

		dispatcher.dispatch(std::move(event));
        return 1;
    }

    case XCB_ENTER_NOTIFY:
    {
		auto& enter = reinterpret_cast<xcb_enter_notify_event_t&>(ev);
		EventHandlerEvent(MouseCrossEvent, enter.event);
        event->position = Vec2i(enter.event_x, enter.event_y);
		event->entered = 1;
		dispatcher.dispatch(std::move(event));

        return 1;
    }

    case XCB_LEAVE_NOTIFY:
    {
		auto& leave = reinterpret_cast<xcb_enter_notify_event_t&>(ev);
		EventHandlerEvent(MouseCrossEvent, leave.event);
        event->position = Vec2i(leave.event_x, leave.event_y);
		event->entered = 0;
		dispatcher.dispatch(std::move(event));

        return 1;
    }

    case XCB_FOCUS_IN:
    {
		auto& focus = reinterpret_cast<xcb_focus_in_event_t&>(ev);
		EventHandlerEvent(FocusEvent, focus.event);
		event->gained = 1;
		dispatcher.dispatch(std::move(event));

        return 1;
    }

    case XCB_FOCUS_OUT:
    {
		auto& focus = reinterpret_cast<xcb_focus_in_event_t&>(ev);
		EventHandlerEvent(FocusEvent, focus.event);
		event->gained = 0;
		dispatcher.dispatch(std::move(event));

        return 1;
    }

    case XCB_KEY_PRESS:
    {
		auto& key = reinterpret_cast<xcb_key_press_event_t&>(ev);
		XKeyEvent xkey {};
		xkey.keycode = key.detail;
		xkey.state = key.state;
		xkey.display = xDisplay_;

        KeySym keysym;
        char buffer[5];
        XLookupString(&xkey, buffer, 5, &keysym, nullptr);

		EventHandlerEvent(KeyEvent, key.event);
		event->pressed = 1;
		event->key = x11ToKey(keysym);
		event->text = buffer;
		dispatcher.dispatch(std::move(event));

        return 1;
    }

    case XCB_KEY_RELEASE:
    {
		auto& key = reinterpret_cast<xcb_key_press_event_t&>(ev);
		XKeyEvent xkey {};
		xkey.keycode = key.detail;
		xkey.state = key.state;
		xkey.display = xDisplay_;

        KeySym keysym;
        char buffer[5];
        XLookupString(&xkey, buffer, 5, &keysym, nullptr);

		EventHandlerEvent(KeyEvent, key.event);
		event->pressed = 0;
		event->key = x11ToKey(keysym);
		event->text = buffer;
		dispatcher.dispatch(std::move(event));

        return 1;
    }

    case XCB_CONFIGURE_NOTIFY:
	{
		auto& configure = (xcb_configure_notify_event_t &)ev;

        //todo: something about window state
        auto nsize = Vec2ui(configure.width, configure.height);
        auto npos = Vec2i(configure.x, configure.y); //positionEvent

        if(!eventHandler(configure.window))
            return 1;

        //if(any(windowContext(configure.window)->window().size() != nsize)) //sizeEvent
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

        //if(any(windowContext(configure.window)->window().position() != npos))
		{
			EventHandlerEvent(PositionEvent, configure.window);
			event->position = npos;
			event->change = 0;
			dispatcher.dispatch(std::move(event));
		}

        return 1;
    }

    case XCB_CLIENT_MESSAGE:
    {
		auto& client = reinterpret_cast<xcb_client_message_event_t&>(ev);
        if((unsigned long)client.data.data32[0] == atom("WM_DELETE_WINDOW"))
        {
			EventHandlerEvent(CloseEvent, client.window);
			dispatcher.dispatch(std::move(event));

            return 1;
        }
	}

	default:
	{
		XLockDisplay(xDisplay_);
	    auto proc = XESetWireToEvent(xDisplay_, ev.response_type & ~0x80, nullptr);
	    if(proc)
		{
	        XESetWireToEvent(xDisplay_, ev.response_type & ~0x80, proc);
	        XEvent dummy;
	        ev.sequence = LastKnownRequestProcessed(xDisplay_);
	        if(proc(xDisplay_, &dummy, (xEvent*) &ev)) //not handled
			{
				//TODO
			}
	    }
		XUnlockDisplay(xDisplay_);
	}

	}

    return 1;
	#undef EventHandlerEvent
}

bool X11AppContext::dispatchEvents(EventDispatcher& dispatcher)
{
	xcb_generic_event_t* ev;
	while((ev = xcb_poll_for_event(xConnection_)))
	{
		processEvent(*ev, dispatcher);
		free(ev);
	}

	if(xcb_connection_has_error(xConnection_))
	{
		return 0;
	}

	return 1;
}

bool X11AppContext::dispatchLoop(EventDispatcher& dispatcher, LoopControl& control)
{
	std::atomic<bool> run {true};
	control.impl_ = std::make_unique<LoopControlImpl>(run, xConnection_, xDummyWindow_);

	while(run.load())
	{
		xcb_generic_event_t* event = xcb_wait_for_event(xConnection_);
		if(!event) return false;

		processEvent(*event, dispatcher);
		free(event);
	}

	return true;
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

xcb_atom_t X11AppContext::atom(const std::string& name)
{
	if(atoms_.find(name) == atoms_.end())
	{
		auto cookie = xcb_intern_atom(xConnection_, 0, name.size(), name.c_str());
		auto reply = xcb_intern_atom_reply(xConnection_, cookie, nullptr);
		atoms_[name] = reply->atom;
	}
	
	return atoms_[name];
}


/*
void x11AppContext::setClipboard(dataObject& obj)
{
    XSetSelectionOwner(xDisplay_, x11::Clipboard, selectionWindow_, CurrentTime);

    clipboardPaste_ = &obj;
}

bool x11AppContext::getClipboard(dataTypes types, std::function<void(dataObject*)> Callback)
{
    Window w = XGetSelectionOwner(xDisplay_, x11::Clipboard);
    if(!w)
        return 0;

    clipboardRequest_ = 1;
    clipboardCallback_ = Callback;
    clipboardTypes_ = types;

    XConvertSelection(xDisplay_, x11::Clipboard, x11::Targets, x11::Clipboard, selectionWindow_, CurrentTime);

    return 1;
}
*/

/*
    case ReparentNotify: //nothing similar in other backend. done diRectly
    {
        if(handler(ev.xreparent.window))
		{
			auto event = std::make_unique<X11ReparentEvent>(handler(ev.xreparent.window));
			event->event = ev.xreparent;
			nyMainApp()->dispatch(std::move(event));
		}

        return 1;
    }
    case SelectionNotify:
    {
        std::cout << "selectionNotify" << std::endl;

        if(ev.xselection.target == x11::Targets)
        {
            if(clipboardRequest_)
            {

                Property prop = read_property(disp, w, sel);

            }
        }
    }

    case SelectionClear:
    {
        std::cout << "selectionClear" << std::endl;
    }

    case SelectionRequest:
    {

        std::cout << "selectionRequest: " << XGetAtomName(xDisplay_, ev.xselectionrequest.target) << " " <<  XGetAtomName(xDisplay_, ev.xselectionrequest.property) << " " <<  XGetAtomName(xDisplay_, ev.xselectionrequest.selection) << std::endl;

        if(ev.xselectionrequest.selection == x11::Clipboard && ev.xselectionrequest.target == x11::Targets)
        {
            unsigned long data[] = {x11::TypeUTF8, x11::TypeText};
            XChangeProperty(xDisplay_, ev.xselectionrequest.requestor, ev.xselectionrequest.property, ev.xselectionrequest.target, 32, PropModeReplace, (unsigned char*) data, 2);

            XEvent m;
            std::memset(&m, sizeof(m), 0);
            m.xselection.type = SelectionNotify;
            m.xselection.display = xDisplay_;
            m.xselection.time = ev.xselectionrequest.time;
            m.xselection.selection = ev.xselectionrequest.selection;
            m.xselection.target = x11::TypeUTF8;
            m.xselection.property = ev.xselectionrequest.property;
            m.xselection.requestor = ev.xselectionrequest.requestor;

            XSendEvent(xDisplay_, ev.xselectionrequest.requestor, False, 0, &m);
        }

        if(ev.xselectionrequest.selection == x11::Clipboard && ev.xselectionrequest.target != x11::Targets)
        {
            std::cout << "yo" << std::endl;

            XChangeProperty(xDisplay_, ev.xselectionrequest.requestor, ev.xselectionrequest.property, ev.xselectionrequest.target, sizeof(char) * 8, PropModeReplace, (unsigned char*) std::string("pimmel").c_str(), 7);

            XEvent m;
            std::memset(&m, sizeof(m), 0);
            m.xselection.type = SelectionNotify;
            m.xselection.display = xDisplay_;
            m.xselection.time = ev.xselectionrequest.time;
            m.xselection.selection = ev.xselectionrequest.selection;
            m.xselection.target = x11::TypeUTF8;
            m.xselection.property = ev.xselectionrequest.property;
            m.xselection.requestor = ev.xselectionrequest.requestor;

            XSendEvent(xDisplay_, ev.xselectionrequest.requestor, False, 0, &m);
        }


    }
	*/
		/*
        if(ev.xclient.message_type == x11::DndEnter)
        {
            //bool moreThan3 = ev.xclient.data.l[1] & 1;
            return 1;
        }

        else if(ev.xclient.message_type == x11::DndPosition)
        {
            return 1;
        }

        else if(ev.xclient.message_type == x11::DndLeave)
        {
            return 1;
        }

        else if(ev.xclient.message_type == x11::DndDrop)
        {
            dataObject* object = new x11DataObject();
            dataReceiveEvent e(*object);
            x11WC* w = getWindowContext(ev.xclient.window);
            if(!w) return 1;
            nyMainApp()->sendEvent(e, w->getWindow());
            return 1;

        }
*/
}
