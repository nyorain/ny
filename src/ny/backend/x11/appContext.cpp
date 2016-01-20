#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/x11/util.hpp>
#include <ny/app/app.hpp>
#include <ny/window/window.hpp>
#include <ny/window/windowEvents.hpp>

#include <nytl/vec.hpp>
#include <nytl/misc.hpp>
#include <nytl/log.hpp>

#include <X11/Xlibint.h>
#include <cstring>

namespace ny
{

//appContext
X11AppContext::X11AppContext()
{
    //XInitThreads(); //todo, make this optional

    xDisplay_ = XOpenDisplay(nullptr);
    if(!xDisplay_)
    {
        throw std::runtime_error("could not connect to X Server");
        return;
    }

    xDefaultScreenNumber_ = DefaultScreen(xDisplay_);
    xDefaultScreen_ = XScreenOfDisplay(xDisplay_, xDefaultScreenNumber_);

 	xConnection_ = XGetXCBConnection(xDisplay_);
    if(!xConnection_)
    {
		throw 0;
    }

    XSetEventQueueOwner(xDisplay_, XCBOwnsEventQueue);

    //selection events will be sent to this window -> they need no window argument
    //does not need to be mapped
    selectionWindow_ = XCreateSimpleWindow(xDisplay_, DefaultRootWindow(xDisplay_), 0, 0, 
			100, 100, 0, BlackPixel(xDisplay_, xDefaultScreenNumber_), 
			BlackPixel(xDisplay_, xDefaultScreenNumber_));

	//atoms
    const char* names[] = {
        "WM_DELETE_WINDOW",
        "_MOTIF_WM_HINTS",

        "_NET_WM_STATE",
        "_NET_WM_STATE_MAXIMIZED_HORZ",
        "_NET_WM_STATE_MAXIMIZED_VERT",
        "_NET_WM_STATE_FULLSCREEN",
        "_NET_WM_STATE_MODAL",
        "_NET_WM_STATE_HIDDEN",
        "_NET_WM_STATE_STICKY",
        "_NET_WM_STATE_ABOVE",
        "_NET_WM_STATE_BELOW",
        "_NET_WM_STATE_DEMANDS_ATTENTION",
        "_NET_WM_STATE_FOCUSED",
        "_NET_WM_STATE_SKIP_PAGER",
        "_NET_WM_STATE_SKIP_TASKBAR",
        "_NET_WM_STATE_SHADED",

        "_NET_WM_ALLOWED_ACTIONS",
        "_NET_WM_ACTIONS_MINIMIZE",
        "_NET_WM_ACTIONS_MAX_HORZ",
        "_NET_WM_ACTIONS_MAX_VERT",
        "_NET_WM_ACTIONS_MOVE",
        "_NET_WM_ACTIONS_RESIZE",
        "_NET_WM_ACTIONS_CLOSE",
        "_NET_WM_ACTIONS_FULLSCREEN",
        "_NET_WM_ACTIONS_ABOVE",
        "_NET_WM_ACTIONS_BELOW",
        "_NET_WM_ACTIONS_CHANGE_DESKTOP",
        "_NET_WM_ACTIONS_SHADE",
        "_NET_WM_ACTIONS_STICK",

        "_NET_WM_WINDOW_TYPE",
        "_NET_WM_WINDOW_TYPE_DESKTOP",
        "_NET_WM_WINDOW_TYPE_DOCK",
        "_NET_WM_WINDOW_TYPE_TOOLBAR",
        "_NET_WM_WINDOW_TYPE_MENU",
        "_NET_WM_WINDOW_TYPE_UTILITY",
        "_NET_WM_WINDOW_TYPE_SPLASH",
        "_NET_WM_WINDOW_TYPE_DIALOG",
        "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
        "_NET_WM_WINDOW_TYPE_POPUP_MENU",
        "_NET_WM_WINDOW_TYPE_TOOLTIP",
        "_NET_WM_WINDOW_TYPE_NOTIFICATION",
        "_NET_WM_WINDOW_TYPE_COMBO",
        "_NET_WM_WINDOW_TYPE_DND",
        "_NET_WM_WINDOW_TYPE_NORMAL",

        "_NET_FRAME_EXTENTS",
        "_NET_STRUT",
        "_NET_STRUT_PARTIAL",
        "_NET_WM_MOVERESIZE",
        "_NET_DESKTOP"

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
        "UTF8_STRING",

        "_NET_WM_ICON",

        "CARDINAL"
    };

    unsigned int count = sizeof(names) / sizeof(char*);
    Atom ret[count];

    XInternAtoms(xDisplay_, (char**) names, count, False, ret);

    unsigned int i = 0;

    x11::WindowDelete = ret[i++];
    x11::MwmHints = ret[i++];

    x11::State = ret[i++];
    x11::StateMaxHorz = ret[i++];
    x11::StateMaxVert = ret[i++];
    x11::StateFullscreen = ret[i++];
    x11::StateModal = ret[i++];
    x11::StateHidden = ret[i++];
    x11::StateSticky = ret[i++];
    x11::StateAbove = ret[i++];
    x11::StateBelow = ret[i++];
    x11::StateDemandAttention = ret[i++];
    x11::StateFocused = ret[i++];
    x11::StateSkipPager = ret[i++];
    x11::StateSkipTaskbar = ret[i++];
    x11::StateShaded = ret[i++];

    x11::AllowedActions = ret[i++];
    x11::AllowedActionMinimize = ret[i++];
    x11::AllowedActionMaximizeHorz = ret[i++];
    x11::AllowedActionMaximizeVert = ret[i++];
    x11::AllowedActionMove = ret[i++];
    x11::AllowedActionResize = ret[i++];
    x11::AllowedActionClose = ret[i++];
    x11::AllowedActionFullscreen = ret[i++];
    x11::AllowedActionAbove = ret[i++];
    x11::AllowedActionBelow = ret[i++];
    x11::AllowedActionChangeDesktop = ret[i++];
    x11::AllowedActionShade = ret[i++];
    x11::AllowedActionStick = ret[i++];

    x11::Type = ret[i++];
    x11::TypeDesktop = ret[i++];
    x11::TypeDock = ret[i++];
    x11::TypeToolbar = ret[i++];
    x11::TypeMenu = ret[i++];
    x11::TypeUtility = ret[i++];
    x11::TypeSplash = ret[i++];
    x11::TypeDialog = ret[i++];
    x11::TypeDropdownMenu = ret[i++];
    x11::TypePopupMenu = ret[i++];
    x11::TypeTooltip = ret[i++];
    x11::TypeNotification = ret[i++];
    x11::TypeCombo = ret[i++];
    x11::TypeDnd = ret[i++];
    x11::TypeNormal = ret[i++];

    x11::FrameExtents = ret[i++];
    x11::Strut = ret[i++];
    x11::StrutPartial = ret[i++];
    x11::MoveResize = ret[i++];

    x11::DndEnter = ret[i++];
    x11::DndPosition = ret[i++];
    x11::DndStatus = ret[i++];
    x11::DndTypeList = ret[i++];
    x11::DndActionCopy = ret[i++];
    x11::DndDrop = ret[i++];
    x11::DndLeave = ret[i++];
    x11::DndFinished = ret[i++];
    x11::DndSelection = ret[i++];
    x11::DndProxy = ret[i++];
    x11::DndAware = ret[i++];

    x11::Primary = ret[i++];
    x11::Clipboard = ret[i++];
    x11::Targets = ret[i++];

    x11::TypeText = ret[i++];
    x11::TypeUTF8 = ret[i++];

    x11::WMIcon = ret[i++];

    x11::Cardinal = ret[i++];
}

X11AppContext::~X11AppContext()
{
    if(xDisplay_)
	{
		XFlush(xDisplay_);
		XCloseDisplay(xDisplay_);
		xDisplay_ = nullptr;
	}	
}

Window* X11AppContext::handler(XWindow w)
{
    auto* wc = windowContext(w);
    return wc ? &wc->window() : nullptr;
}

void X11AppContext::sendRedrawEvent(XWindow w)
{
    auto* wc = windowContext(w);
    if(wc) nyMainApp()->dispatch(make_unique<DrawEvent>(&wc->window()));
    return;
}

bool X11AppContext::processEvent(xcb_generic_event_t& ev)
{
	auto responseType = ev.response_type & ~0x80;
    switch(responseType)
    {
    case XCB_MOTION_NOTIFY:
    {
		auto& motion = reinterpret_cast<xcb_motion_notify_event_t&>(ev);  
		auto event = make_unique<MouseMoveEvent>(handler(motion.event));
        event->position = vec2i(motion.event_x, motion.event_y);
        event->screenPosition = vec2i(motion.root_x, motion.root_y);
        event->delta = event->position - Mouse::position();

		nyMainApp()->dispatch(std::move(event));
        return 1;
    }

    case XCB_EXPOSE:
    {
		auto& expose = (xcb_expose_event_t &)ev;  
        if(expose.count == 0) sendRedrawEvent(expose.window);
        return 1;
    }
    case XCB_MAP_NOTIFY:
    {
		auto& map = (xcb_map_notify_event_t &)ev;  
        sendRedrawEvent(map.window);
        return 1;
    }

    case XCB_BUTTON_PRESS:
    {
		auto& button = reinterpret_cast<xcb_button_press_event_t&>(ev);  
		auto event = make_unique<MouseButtonEvent>(handler(button.event));
		event->data = make_unique<X11EventData>(ev);
        event->button = x11ToButton(button.detail);
        event->position = vec2i(button.event_x, button.event_y);
		event->pressed = 1;

        nyMainApp()->dispatch(std::move(event));
        return 1;
    }

    case XCB_BUTTON_RELEASE:
    {
		auto& button = reinterpret_cast<xcb_button_release_event_t&>(ev);  
		auto event = make_unique<MouseButtonEvent>(handler(button.event));
		event->data = make_unique<X11EventData>(ev);
        event->button = x11ToButton(button.detail);
        event->position = vec2i(button.event_x, button.event_y);
		event->pressed = 0;

        nyMainApp()->dispatch(std::move(event));
        return 1;
    }

    case XCB_ENTER_NOTIFY:
    {
		auto& enter = reinterpret_cast<xcb_enter_notify_event_t&>(ev);  
		auto event = make_unique<MouseCrossEvent>(handler(enter.event));
        event->position = vec2i(enter.event_x, enter.event_y);
		event->entered = 1;
        nyMainApp()->dispatch(std::move(event));

        return 1;
    }

    case XCB_LEAVE_NOTIFY:
    {
		auto& leave = reinterpret_cast<xcb_enter_notify_event_t&>(ev);  
		auto event = make_unique<MouseCrossEvent>(handler(leave.event));
        event->position = vec2i(leave.event_x, leave.event_y);
		event->entered = 0;
        nyMainApp()->dispatch(std::move(event));

        return 1;
    }

    case XCB_FOCUS_IN:
    {
		auto& focus = reinterpret_cast<xcb_focus_in_event_t&>(ev);  
		auto event = make_unique<FocusEvent>(handler(focus.event));
		event->focusGained = 1;
        nyMainApp()->dispatch(std::move(event));

        return 1;
    }

    case XCB_FOCUS_OUT:
    {
		auto& focus = reinterpret_cast<xcb_focus_in_event_t&>(ev);  
		auto event = make_unique<FocusEvent>(handler(focus.event));
		event->focusGained = 0;
        nyMainApp()->dispatch(std::move(event));

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

		auto event = make_unique<KeyEvent>(handler(key.event));
		event->pressed = 1;
		event->key = x11ToKey(keysym);
		event->text = buffer;
        nyMainApp()->dispatch(std::move(event));

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

		auto event = make_unique<KeyEvent>(handler(key.event));
		event->pressed = 0;
		event->key = x11ToKey(keysym);
		event->text = buffer;
        nyMainApp()->dispatch(std::move(event));

        return 1;
    }

    case XCB_CONFIGURE_NOTIFY:
	{
		auto& configure = (xcb_configure_notify_event_t &)ev;  

        //todo: something about window state
        auto nsize = vec2ui(configure.width, configure.height);
        auto npos = vec2i(configure.x, configure.y); //positionEvent

        if(!handler(configure.window))
            return 1;

        if(any(windowContext(configure.window)->window().size() != nsize)) //sizeEvent
		{
			auto event = make_unique<SizeEvent>(handler(configure.window));
			event->size = nsize;
			event->change = 0;
			nyMainApp()->dispatch(std::move(event));
		}

        if(any(windowContext(configure.window)->window().position() != npos))
		{
			auto event = make_unique<PositionEvent>(handler(configure.window));
			event->position = npos;
			event->change = 0;
			nyMainApp()->dispatch(std::move(event));
		}

        return 1;
    }
/*
    case ReparentNotify: //nothing similar in other backend. done directly
    {
        if(handler(ev.xreparent.window))
		{
			auto event = make_unique<X11ReparentEvent>(handler(ev.xreparent.window));
			event->event = ev.xreparent;
			nyMainApp()->dispatch(std::move(event));
		}

        return 1;
    }
*/
    case SelectionNotify:
    {
        /*
        std::cout << "selectionNotify" << std::endl;

        if(ev.xselection.target == x11::Targets)
        {
            if(clipboardRequest_)
            {

                Property prop = read_property(disp, w, sel);

            }
        }
        */
    }

    case SelectionClear:
    {
        //std::cout << "selectionClear" << std::endl;
    }

    case SelectionRequest:
    {
        /*
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
        */

    }

    case ClientMessage:
    {
		auto& client = (xcb_client_message_event_t&)ev;
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
        if((unsigned long)client.data.data32[0] == x11::WindowDelete)
        {
            if(handler(client.window)) 
			{
				auto event = make_unique<DestroyEvent>(handler(client.window));
				nyMainApp()->dispatch(std::move(event));
			}

            return 1;
        }
    }

	}

    XLockDisplay(xDisplay_);
    Bool(*proc)(Display*, XEvent*, xEvent*) = 
		XESetWireToEvent(xDisplay_, ev.response_type & ~0x80, nullptr);
    if(proc) 
	{
        XESetWireToEvent(xDisplay_, ev.response_type & ~0x80, proc);
        XEvent dummy;
        ev.sequence = LastKnownRequestProcessed(xDisplay_);
        if(proc(xDisplay_, &dummy, (xEvent*) &ev)) //not handled
		{
			//goddamit
		}
    }
	XUnlockDisplay(xDisplay_);


    return 1;
}

int X11AppContext::mainLoop()
{
	//XSetWindowAttributes attr;
	//attr.event_mask = SubstructureNotifyMask;
	//XChangeWindowAttributes(xDisplay_, DefaultRootWindow(xDisplay_), CWEventMask, &attr);

	runMainLoop_ = 1;
	while(runMainLoop_)
	{
		xcb_generic_event_t *event = xcb_wait_for_event(xConnection_);
		processEvent(*event);
		free(event);
	}
	
	return 1;
}

void X11AppContext::exit()
{
	runMainLoop_ = 0;
    XFlush(xDisplay_);
}

/*
void x11AppContext::setClipboard(dataObject& obj)
{
    XSetSelectionOwner(xDisplay_, x11::Clipboard, selectionWindow_, CurrentTime);

    clipboardPaste_ = &obj;
}

bool x11AppContext::getClipboard(dataTypes types, std::function<void(dataObject*)> callback)
{
    Window w = XGetSelectionOwner(xDisplay_, x11::Clipboard);
    if(!w)
        return 0;

    clipboardRequest_ = 1;
    clipboardCallback_ = callback;
    clipboardTypes_ = types;

    XConvertSelection(xDisplay_, x11::Clipboard, x11::Targets, x11::Clipboard, selectionWindow_, CurrentTime);

    return 1;
}
*/

void X11AppContext::registerContext(XWindow w, X11WindowContext& c)
{
    contexts_[w] = &c;
}

void X11AppContext::unregisterContext(XWindow w)
{
    if(contexts_.find(w) != contexts_.end())
        contexts_[w] = nullptr;
}

X11WindowContext* X11AppContext::windowContext(XWindow win)
{
    if(contexts_.find(win) != contexts_.end())
        return contexts_[win];

    return nullptr;
}

/////
Display* xDisplay()
{
    X11AppContext* a;
    if(!nyMainApp() || !(a = asX11(&nyMainApp()->appContext())))
        return nullptr;

    return a->xDisplay();
}

X11AppContext* x11AppContext()
{
    X11AppContext* ret = nullptr;

    if(nyMainApp())
    {
        ret = asX11(&nyMainApp()->appContext());
    }

    return ret;
}

}
