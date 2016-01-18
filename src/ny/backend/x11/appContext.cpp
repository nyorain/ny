#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/x11/util.hpp>
#include <ny/app/app.hpp>
#include <ny/window/window.hpp>
#include <ny/window/windowEvents.hpp>

#include <nytl/vec.hpp>
#include <nytl/misc.hpp>
#include <nytl/log.hpp>

#include <cstring>

namespace ny
{

//appContext
X11AppContext::X11AppContext()
{
    XInitThreads(); //todo, make this optional

    xDisplay_ = XOpenDisplay(nullptr);
    if(!xDisplay_)
    {
        throw std::runtime_error("could not connect to X Server");
        return;
    }

    xDefaultScreenNumber_ = DefaultScreen(xDisplay_);
    xDefaultScreen_ = XScreenOfDisplay(xDisplay_, xDefaultScreenNumber_);

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

bool X11AppContext::processEvent(XEvent& ev)
{
    switch(ev.type)
    {
    case MotionNotify:
    {
		auto event = make_unique<MouseMoveEvent>(handler(ev.xmotion.window));
        event->position = vec2i(ev.xmotion.x, ev.xmotion.y);
        event->screenPosition = vec2i(ev.xmotion.x_root, ev.xmotion.y_root);
        event->delta = event->position - Mouse::position();

		nyMainApp()->dispatch(std::move(event));
        return 1;
    }

    case Expose:
    {
        if(ev.xexpose.count == 0) sendRedrawEvent(ev.xexpose.window);
        return 1;
    }

    case MapNotify:
    {
        sendRedrawEvent(ev.xmap.window);
        return 1;
    }

    case ButtonPress:
    {
		auto event = make_unique<MouseButtonEvent>(handler(ev.xbutton.window));
		event->data = make_unique<X11EventData>(ev);
        event->button = x11ToButton(ev.xbutton.button);
        event->position = vec2i(ev.xbutton.x, ev.xbutton.y);
		event->pressed = 1;

        nyMainApp()->dispatch(std::move(event));
        return 1;
    }

    case ButtonRelease:
    {
		auto event = make_unique<MouseButtonEvent>(ev.xbutton.window);
		event->data = make_unique<X11EventData>(ev);
        event->button = x11ToButton(ev.xbutton.button);
        event->position = vec2i(ev.xbutton.x, ev.xbutton.y);
		event->pressed = 0;

        nyMainApp()->dispatch(std::move(event));
        return 1;
    }

    case EnterNotify:
    {
		auto event = make_unique<MouseCrossEvent>(handler(ev.xcrossing.window));
        event->position = vec2i(ev.xcrossing.x, ev.xcrossing.y);
		event->entered = 1;
        nyMainApp()->dispatch(std::move(event));

        return 1;
    }

    case LeaveNotify:
    {
		auto event = make_unique<MouseCrossEvent>(handler(ev.xcrossing.window));
        event->position = vec2i(ev.xcrossing.x, ev.xcrossing.y);
		event->entered = 0;
        nyMainApp()->dispatch(std::move(event));

        return 1;
    }

    case FocusIn:
    {
		auto event = make_unique<FocusEvent>(handler(ev.xcrossing.window));
		event->focusGained = 1;
        nyMainApp()->dispatch(std::move(event));

        return 1;
    }

    case FocusOut:
    {
		auto event = make_unique<FocusEvent>(handler(ev.xcrossing.window));
		event->focusGained = 0;
        nyMainApp()->dispatch(std::move(event));

        return 1;
    }

    case KeyPress:
    {
        KeySym keysym;
        char buffer[5];
        XLookupString(&ev.xkey, buffer, 5, &keysym, nullptr);

		auto event = make_unique<KeyEvent>(handler(ev.xkey.window));
		event->pressed = 1;
		event->key = x11ToKey(keysym);
		event->text = buffer;
        nyMainApp()->dispatch(std::move(event));
        return 1;
    }

    case KeyRelease:
    {
        KeySym keysym;
        char buffer[5];
        XLookupString(&ev.xkey, buffer, 5, &keysym, nullptr);

		auto event = make_unique<KeyEvent>(handler(ev.xkey.window));
		event->pressed = 0;
		event->key = x11ToKey(keysym);
		event->text = buffer;
        nyMainApp()->dispatch(std::move(event));
        return 1;
    }

    case ConfigureNotify:
    {
        //todo: something about window state
        auto nsize = vec2ui(ev.xconfigure.width, ev.xconfigure.height);
        auto npos = vec2i(ev.xconfigure.x, ev.xconfigure.y); //positionEvent

        if(!handler(ev.xconfigure.window))
            return 1;

        if(any(windowContext(ev.xconfigure.window)->window().size() != nsize)) //sizeEvent
		{
			auto event = make_unique<SizeEvent>(handler(ev.xconfigure.window));
			event->size = nsize;
			event->change = 0;
			nyMainApp()->dispatch(std::move(event));
		}

        if(any(windowContext(ev.xconfigure.window)->window().position() != npos))
		{
			auto event = make_unique<PositionEvent>(handler(ev.xconfigure.window));
			event->position = npos;
			event->change = 0;
			nyMainApp()->dispatch(std::move(event));
		}

        return 1;
    }

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
            /*
            dataObject* object = new x11DataObject();
            dataReceiveEvent e(*object);
            x11WC* w = getWindowContext(ev.xclient.window);
            if(!w) return 1;
            nyMainApp()->sendEvent(e, w->getWindow());
            return 1;
            */
        }

        else if((unsigned long)ev.xclient.data.l[0] == x11::WindowDelete)
        {
            if(handler(ev.xclient.window)) 
			{
				auto event = make_unique<DestroyEvent>(handler(ev.xclient.window));
				nyMainApp()->dispatch(std::move(event));
			}

            return 1;
        }
    }

    }

    return 1;
}

int X11AppContext::mainLoop()
{
	runMainLoop_ = 1;
	while(runMainLoop_)
	{
		XEvent event;
		XNextEvent(xDisplay_, &event);
		processEvent(event);
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
