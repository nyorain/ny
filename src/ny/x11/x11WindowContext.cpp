#include <ny/x11/x11WindowContext.hpp>

#include <ny/x11/x11Util.hpp>
#include <ny/x11/x11AppContext.hpp>

#include <ny/app.hpp>
#include <ny/window.hpp>
#include <ny/event.hpp>
#include <ny/error.hpp>
#include <ny/cursor.hpp>
#include <ny/image.hpp>

#include <X11/Xatom.h>

#include <cairo/cairo-xlib.h>

#include <memory.h>
#include <iostream>

namespace ny
{

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//windowContext///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
x11WindowContext::x11WindowContext(window& win, const x11WindowContextSettings& settings) : windowContext(win, settings), context_(nullptr), xDisplay_(nullptr), xWindow_(0), xVinfo_(nullptr), mwmFuncHints_(0), mwmDecoHints_(0)
{
    context_ = asX11(getMainApp()->getAppContext());

    if(!context_)
    {
        throw std::runtime_error("x11 App was not correctly initialized");
        return;
    }

    xDisplay_ = context_->getXDisplay();

    if(!xDisplay_)
    {
        throw std::runtime_error("x11 App was not correctly initialized");
        return;
    }

    xScreenNumber_ = context_->getXDefaultScreenNumber(); //todo: implement correctly
}

x11WindowContext::~x11WindowContext()
{
    context_->unregisterContext(xWindow_);

    XDestroyWindow(xDisplay_, xWindow_);
}

void x11WindowContext::create(unsigned int winType)
{
    if(!xVinfo_)
    {
        if(!matchVisualInfo())
        {
            throw std::runtime_error("could not match visual");
            return;
        }
    }

    unsigned long mask = CWColormap | CWEventMask;

    XSetWindowAttributes attr;
    attr.colormap = XCreateColormap(xDisplay_, DefaultRootWindow(xDisplay_), xVinfo_->visual, AllocNone);
    attr.event_mask = eventMapToX11(window_.getEventMap());

    if(window_.getCursor().isNativeType())
    {
        int xCursor = cursorToX11(window_.getCursor().getNativeType());
        if(xCursor != -1)
        {
            attr.cursor = XCreateFontCursor(xDisplay_, xCursor);
            mask |= CWCursor;
        }
    }
    //todo: cursor image

    if(!hasGL())
    {
        attr.background_pixel = 0;
        attr.border_pixel = 0;

        mask |= CWBackPixel | CWBorderPixel;
    }

    create(winType, mask, attr);
}

bool x11WindowContext::matchVisualInfo()
{
    if(!xVinfo_)xVinfo_ = new XVisualInfo;

    //todo: other visuals possible
    if(!XMatchVisualInfo(xDisplay_, context_->getXDefaultScreenNumber(), 32, TrueColor, xVinfo_))
    {
        return 0;
    }

    return 1;
}

void x11WindowContext::refresh()
{
    //event-method
    /*
    drawEvent* e = new drawEvent;
    e->handler = &window_;
    e->backend = 0;
    getMainApp()->windowDraw(e);
    */

    XEvent ev;

    memset(&ev, 0, sizeof(ev));

    ev.type = Expose;
    ev.xexpose.window = xWindow_;

    XSendEvent(xDisplay_, xWindow_, False, ExposureMask, &ev);
    XFlush(xDisplay_);
}

void x11WindowContext::finishDraw()
{
    XFlush(xDisplay_);
}

void x11WindowContext::show()
{
    XMapWindow(xDisplay_, xWindow_);
}

void x11WindowContext::hide()
{
    XUnmapWindow(xDisplay_, xWindow_);
}

void x11WindowContext::raise()
{
    XRaiseWindow(xDisplay_, xWindow_);
}

void x11WindowContext::lower()
{
    XLowerWindow(xDisplay_, xWindow_);
}

void x11WindowContext::requestFocus()
{
    addState(x11::StateFocused); //todo: fix
}

void x11WindowContext::setSize(vec2ui size, bool change)
{
    if(change)
    {
        XResizeWindow(xDisplay_, xWindow_, size.x, size.y);
    }
}

void x11WindowContext::setPosition(vec2i position, bool change)
{
    if(change)
    {
        XMoveWindow(xDisplay_, xWindow_, position.x, position.y);
    }
}

void x11WindowContext::setCursor(const cursor& curs)
{
    if(curs.isNativeType())
    {
        int num = cursorToX11(curs.getNativeType());

        if(num != -1)
        {
            setCursor(num);
        }
    }
    //todo: image
}

void x11WindowContext::setMinSize(vec2ui size)
{
    long a;
    XSizeHints s;
    XGetWMNormalHints(xDisplay_, xWindow_, &s, &a);
    s.min_width = size.x;
    s.min_height = size.y;
    s.flags |= PMinSize;
    XSetWMNormalHints(xDisplay_, xWindow_, &s);
}

void x11WindowContext::setMaxSize(vec2ui size)
{
    long a;
    XSizeHints s;
    XGetWMNormalHints(xDisplay_, xWindow_, &s, &a);
    s.max_width = size.x;
    s.max_height = size.y;
    s.flags |= PMaxSize;
    XSetWMNormalHints(xDisplay_, xWindow_, &s);
}

void x11WindowContext::sendContextEvent(contextEvent& e)
{
    if(e.contextEventType == X11Reparent)
    {
        wasReparented(e.to<x11ReparentEvent>());
    }
}

void x11WindowContext::mapEventType(unsigned int t)
{
    XWindowAttributes getAttr;
    XGetWindowAttributes(xDisplay_, xWindow_, &getAttr);

    long xCode = eventTypeToX11(t);
    if(xCode == -1)
        return;

    XSetWindowAttributes attr;
    attr.event_mask = getAttr.all_event_masks | xCode;
    XChangeWindowAttributes(xDisplay_, xWindow_, CWEventMask, &attr);
}

void x11WindowContext::unmapEventType(unsigned int t)
{
    XWindowAttributes getAttr;
    XGetWindowAttributes(xDisplay_, xWindow_, &getAttr);

    long xCode = eventTypeToX11(t);
    if(xCode == -1)
        return;

    XSetWindowAttributes attr;
    attr.event_mask = getAttr.all_event_masks & ~xCode;
    XChangeWindowAttributes(xDisplay_, xWindow_, CWEventMask, &attr);
}

void x11WindowContext::addWindowHints(unsigned long hints)
{
    unsigned long motifDeco = 0;
    unsigned long motifFunc = 0;


    if(hints & windowHints::Close)
    {
        motifFunc |= x11::MwmFuncClose;
        addAllowedAction(x11::AllowedActionClose);
    }
    if(hints & windowHints::Maximize)
    {
        motifFunc |= x11::MwmFuncMaximize;
        motifDeco |= x11::MwmDecoMaximize;

        addAllowedAction(x11::AllowedActionMaximizeHorz);
        addAllowedAction(x11::AllowedActionMaximizeVert);
    }

    if(hints & windowHints::Minimize)
    {
        motifFunc |= x11::MwmFuncMinimize;
        motifDeco |= x11::MwmDecoMinimize;

        addAllowedAction(x11::AllowedActionMinimize);
    }
    if(hints & windowHints::Move)
    {
        motifFunc |= x11::MwmFuncMove;
        motifDeco |= x11::MwmDecoTitle;

        addAllowedAction(x11::AllowedActionMove);
    }
    if(hints & windowHints::Resize)
    {
        motifFunc |= x11::MwmFuncResize;
        motifDeco |= x11::MwmDecoResize;

        addAllowedAction(x11::AllowedActionResize);
    }
    if(hints & windowHints::ShowInTaskbar)
    {
        removeState(x11::StateSkipPager);
        removeState(x11::StateSkipTaskbar);
    }
    if(hints & windowHints::AlwaysOnTop)
    {
        addState(x11::StateAbove);
    }

    mwmFuncHints_ |= motifFunc;
    mwmDecoHints_ |= motifDeco;

    setMwmHints(mwmDecoHints_, mwmFuncHints_);
}
void x11WindowContext::removeWindowHints(unsigned long hints)
{
    unsigned long motifDeco = 0;
    unsigned long motifFunc = 0;

    if(hints & windowHints::Close)
    {
        motifFunc |= x11::MwmFuncClose;
        removeAllowedAction(x11::AllowedActionClose);
    }
    if(hints & windowHints::Maximize)
    {
        motifFunc |= x11::MwmFuncMaximize;
        motifDeco |= x11::MwmDecoMaximize;

        removeAllowedAction(x11::AllowedActionMaximizeHorz);
        removeAllowedAction(x11::AllowedActionMaximizeVert);
    }

    if(hints & windowHints::Minimize)
    {
        motifFunc |= x11::MwmFuncMinimize;
        motifDeco |= x11::MwmDecoMinimize;

        removeAllowedAction(x11::AllowedActionMinimize);
    }
    if(hints & windowHints::Move)
    {
        motifFunc |= x11::MwmFuncMove;
        motifDeco |= x11::MwmDecoTitle;

        removeAllowedAction(x11::AllowedActionMove);
    }
    if(hints & windowHints::Resize)
    {
        motifFunc |= x11::MwmFuncResize;
        motifDeco |= x11::MwmDecoResize;

        removeAllowedAction(x11::AllowedActionResize);
    }
    if(hints & windowHints::ShowInTaskbar)
    {
        addState(x11::StateSkipPager);
        addState(x11::StateSkipTaskbar);
    }
    if(hints & windowHints::AlwaysOnTop)
    {
        removeState(x11::StateAbove);
    }


    mwmFuncHints_ &= ~motifFunc;
    mwmDecoHints_ &= ~motifDeco;

    setMwmHints(mwmDecoHints_, mwmFuncHints_);
}

void x11WindowContext::addContextHints(unsigned long hints)
{
    windowContext::addContextHints(hints);

    if(hints & x11::hintOverrideRedirect)
    {
        setOverrideRedirect(1);
    }

}
void x11WindowContext::removeContextHints(unsigned long hints)
{
    windowContext::removeContextHints(hints);

    if(hints & x11::hintOverrideRedirect)
    {
        setOverrideRedirect(0);
    }
}


//x11 specific////////////////////////////////////////////////////////////////////////////////////
void x11WindowContext::addState(Atom state)
{
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = xWindow_;
    ev.xclient.message_type = x11::State;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 1; //add
    ev.xclient.data.l[1] = state;
    ev.xclient.data.l[2] = 0;

    XSendEvent(xDisplay_, DefaultRootWindow(xDisplay_), False, SubstructureNotifyMask, &ev);

    states_ |= state;
}

void x11WindowContext::removeState(Atom state)
{
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = xWindow_;
    ev.xclient.message_type = x11::State;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 0; //remove
    ev.xclient.data.l[1] = state;
    ev.xclient.data.l[2] = 0;

    XSendEvent(xDisplay_, DefaultRootWindow(xDisplay_), False, SubstructureNotifyMask, &ev);

    states_ = states_ & ~state;
}

void x11WindowContext::toggleState(Atom state)
{
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = xWindow_;
    ev.xclient.message_type = x11::State;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 2; //toggle
    ev.xclient.data.l[1] = state;
    ev.xclient.data.l[2] = 0;

    XSendEvent(xDisplay_, DefaultRootWindow(xDisplay_), False, SubstructureNotifyMask, &ev);

    states_ = states_ xor state;
}

void x11WindowContext::setMwmHints(unsigned long deco, unsigned long func)
{
    mwmDecoHints_ = deco;
    mwmFuncHints_ = func;

    struct x11::mwmHints mhints;
    mhints.flags = x11::MwmHintsDeco | x11::MwmHintsFunc;
    mhints.decorations = deco;
    mhints.functions = func;
    XChangeProperty(xDisplay_, xWindow_, x11::MwmHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)&mhints, sizeof (x11::mwmHints)/sizeof (long));
}

void x11WindowContext::setMwmDecorationHints(const unsigned long hints)
{
    mwmDecoHints_ = hints;

    struct x11::mwmHints mhints;
    mhints.flags = x11::MwmHintsDeco;
    mhints.decorations = hints;
    XChangeProperty(xDisplay_, xWindow_, x11::MwmHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)&mhints, sizeof (x11::mwmHints)/sizeof (long));
}

void x11WindowContext::setMwmFunctionHints(const unsigned long hints)
{
    mwmFuncHints_ = hints;

    struct x11::mwmHints mhints;
    mhints.flags = x11::MwmHintsFunc;
    mhints.functions = hints;
    XChangeProperty(xDisplay_, xWindow_, x11::MwmHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)&mhints, sizeof (x11::mwmHints)/sizeof (long));
}

unsigned long x11WindowContext::getMwmFunctionHints()
{
    //todo
    return 0;
}

unsigned long x11WindowContext::getMwmDecorationHints()
{
    //todo
    return 0;
}

void x11WindowContext::addAllowedAction(Atom action)
{
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = xWindow_;
    ev.xclient.message_type = x11::AllowedActions;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 1; //add
    ev.xclient.data.l[1] = action;
    ev.xclient.data.l[2] = 0;

    XSendEvent(xDisplay_, DefaultRootWindow(xDisplay_), False, SubstructureNotifyMask, &ev);
}

void x11WindowContext::removeAllowedAction(Atom action)
{
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = xWindow_;
    ev.xclient.message_type = x11::AllowedActions;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 0; //remove
    ev.xclient.data.l[1] = action;
    ev.xclient.data.l[2] = 0;

    XSendEvent(xDisplay_, DefaultRootWindow(xDisplay_), False, SubstructureNotifyMask, &ev);
}

std::vector<Atom> x11WindowContext::getAllowedAction()
{
    std::vector<Atom> ret;
    return ret;

    //todo
}

void x11WindowContext::refreshStates()
{
    //todo
}

void x11WindowContext::setTransientFor(window* w)
{
    x11WC* other = asX11(w->getWindowContext());

    XSetTransientForHint(xDisplay_, other->getXWindow(), xWindow_);
}

void x11WindowContext::setType(const Atom type)
{
    XChangeProperty(xDisplay_, xWindow_, x11::Type, XA_ATOM, 32, PropModeReplace, (unsigned char*) &type, 1);
}

Atom x11WindowContext::getType()
{
    return 0;
    //todo
}

void x11WindowContext::setOverrideRedirect(bool redirect)
{
    XSetWindowAttributes attr;
    attr.override_redirect = redirect;

    XChangeWindowAttributes(xDisplay_, xWindow_, CWOverrideRedirect, &attr);
}

void x11WindowContext::setCursor(unsigned int xCursorID)
{
    Cursor c = XCreateFontCursor(xDisplay_, xCursorID);
    XDefineCursor(xDisplay_, xWindow_, c);
}

void x11WindowContext::wasReparented(x11ReparentEvent& ev)
{
    setPosition(window_.getPosition()); //set position correctly
}

//toplevel//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
x11ToplevelWindowContext::x11ToplevelWindowContext(toplevelWindow& win, const x11WindowContextSettings& settings, bool pcreate) : windowContext(win, settings), toplevelWindowContext(win, settings), x11WindowContext(win, settings)
{
    if(pcreate)
        create();
}

void x11ToplevelWindowContext::create(unsigned int winType, unsigned long attrMask, XSetWindowAttributes attr)
{
    if(!xVinfo_)
    {
        if(!matchVisualInfo())
        {
            throw std::runtime_error("could not match visual");
            return;
        }
    }

    xWindow_ = XCreateWindow(xDisplay_, DefaultRootWindow(xDisplay_), window_.getPositionX(), window_.getPositionY(), window_.getWidth(), window_.getHeight(), getToplevelWindow().getBorderSize(), xVinfo_->depth, winType, xVinfo_->visual, attrMask, &attr);

    //XClassHint hint;
    //hint.res_class = (char*) getToplevelWindow().getName().c_str();
    //hint.res_name = (char*) getToplevelWindow().getName().c_str();
    //XSetClassHint(xDisplay_, xWindow_, &hint);

    XStoreName(xDisplay_, xWindow_, getToplevelWindow().getName().c_str());

    XSetWMProtocols(xDisplay_, xWindow_, &x11::WindowDelete, 1);
    context_->registerContext(xWindow_, this); //like user data for window
}

void x11ToplevelWindowContext::setMaximized()
{
    addState(x11::StateMaxHorz);
    addState(x11::StateMaxVert);
}

void x11ToplevelWindowContext::setMinimized()
{
    XWMHints hints;
    hints.flags = StateHint;
    hints.initial_state = IconicState;
    XSetWMHints(xDisplay_, xWindow_, &hints);
}

void x11ToplevelWindowContext::setFullscreen()
{
    addState(x11::StateFullscreen);
}

void x11ToplevelWindowContext::setNormal()
{
    XWMHints hints;
    hints.flags = StateHint;
    hints.initial_state = NormalState;
    XSetWMHints(xDisplay_, xWindow_, &hints);
}

void x11ToplevelWindowContext::beginMove(mouseButtonEvent* ev)
{
    x11EventData* xbev = dynamic_cast<x11EventData*>(ev->data);
    if(!xbev)
        return;

    XEvent xev = xbev->ev;

    XEvent mev;
    XUngrabPointer(xDisplay_, 0L);

    mev.type = ClientMessage;
    mev.xclient.window = xWindow_;
    mev.xclient.message_type = x11::MoveResize;
    mev.xclient.format = 32;
    mev.xclient.data.l[0] = xev.xbutton.x_root;
    mev.xclient.data.l[1] = xev.xbutton.y_root;
    mev.xclient.data.l[2] = x11::MoveResizeMove;
    mev.xclient.data.l[3] = xev.xbutton.button;
    mev.xclient.data.l[4] = 1; //default. could be set to 2 for pager

    XSendEvent(xDisplay_, DefaultRootWindow(xDisplay_), False, SubstructureNotifyMask , &mev);
}

void x11ToplevelWindowContext::beginResize(mouseButtonEvent* ev, windowEdge edge)
{
    x11EventData* xbev = dynamic_cast<x11EventData*>(ev->data);

    if(!xbev)
        return;

    unsigned long x11Edge = 0;

    switch(edge)
    {
        case windowEdge::Top: x11Edge = x11::MoveResizeSizeTop; break;
        case windowEdge::Left: x11Edge = x11::MoveResizeSizeLeft; break;
        case windowEdge::Bottom: x11Edge = x11::MoveResizeSizeBottom; break;
        case windowEdge::Right: x11Edge = x11::MoveResizeSizeRight; break;
        case windowEdge::TopLeft: x11Edge = x11::MoveResizeSizeTopLeft; break;
        case windowEdge::TopRight: x11Edge = x11::MoveResizeSizeTopRight; break;
        case windowEdge::BottomLeft: x11Edge = x11::MoveResizeSizeBottomLeft; break;
        case windowEdge::BottomRight: x11Edge = x11::MoveResizeSizeBottomRight; break;
        default: return;
    }

    XEvent xev = xbev->ev;

    XEvent mev;
    XUngrabPointer(xDisplay_, 0L);

    mev.type = ClientMessage;
    mev.xclient.window = xWindow_;
    mev.xclient.message_type = x11::MoveResize;
    mev.xclient.format = 32;
    mev.xclient.data.l[0] = xev.xbutton.x_root;
    mev.xclient.data.l[1] = xev.xbutton.y_root;
    mev.xclient.data.l[2] = x11Edge;
    mev.xclient.data.l[3] = xev.xbutton.button;
    mev.xclient.data.l[4] = 1; //default. could be set to 2 for pager

    XSendEvent(xDisplay_, DefaultRootWindow(xDisplay_), False, SubstructureNotifyMask , &mev);
}

void x11ToplevelWindowContext::setBorderSize(unsigned int size)
{
    XSetWindowBorderWidth(xDisplay_, xWindow_, size);
}

void x11ToplevelWindowContext::setIcon(const image* img)
{
    //TODO
    if(img)
    {
        unsigned int length =  2 + img->getSize().x * img->getSize().y;
        unsigned int size = img->getSize().x * img->getSize().y;

        unsigned long buffer[length];
        buffer[0] = img->getSize().x;
        buffer[1] = img->getSize().x;

        const unsigned char* imageData = img->getDataPlain();


        for(unsigned int i(0); i < length - 2; i++)
        {
            buffer[i + 2] = (imageData[size * 3 + i] << 24) | (imageData[i] << 16) | (imageData[size + i] << 8) | (imageData[size * 2 + i] << 0);
        }


        XChangeProperty(getXDisplay(), xWindow_, x11::WMIcon, x11::Cardinal, 32, PropModeReplace, (const unsigned char*) buffer, length);

        return;
    }

    unsigned long buffer[2];

    buffer[0] = 0;
    buffer[1] = 0;

    XChangeProperty(getXDisplay(), xWindow_, x11::WMIcon, x11::Cardinal, 32, PropModeReplace, (const unsigned char*) buffer, 2);


}

//x11ChildWC////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////77
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
x11ChildWindowContext::x11ChildWindowContext(childWindow& win, const x11WindowContextSettings& settings, bool pcreate) : windowContext(win, settings), childWindowContext(win, settings), x11WindowContext(win, settings)
{
    if(pcreate)
        create();
}

void x11ChildWindowContext::create(unsigned int winType, unsigned long attrMask, XSetWindowAttributes attr)
{
    if(!xVinfo_)
    {
        if(!matchVisualInfo())
        {
            throw std::runtime_error("could not match visual");
            return;
        }
    }

    xWindow_ = XCreateWindow(xDisplay_, asX11(window_.getParent()->getWindowContext())->getXWindow(), window_.getPositionX(), window_.getPositionY(), window_.getWidth(), window_.getHeight(), 0, xVinfo_->depth, winType, xVinfo_->visual, attrMask, &attr);

    context_->registerContext(xWindow_, this); //like user data for window
}

}
