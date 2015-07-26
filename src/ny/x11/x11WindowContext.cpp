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

#include <memory.h>
#include <iostream>

#ifdef NY_WithGL
#include <ny/x11/glx.hpp>
#include <GL/glx.h>
#endif // NY_WithGL

#ifdef NY_WithEGL
#include <ny/x11/x11Egl.hpp>
#endif // NY_WithEGL

#ifdef NY_WithCairo
#include <ny/x11/x11Cairo.hpp>
#endif // NY_WithCairo

namespace ny
{

struct glxFBC
{
    GLXFBConfig config;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//windowContext///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
x11WindowContext::x11WindowContext(window& win, const x11WindowContextSettings& settings) : windowContext(win, settings)
{
    x11AppContext* ac = getX11AppContext();

    if(!ac)
    {
        throw std::runtime_error("x11 App was not correctly initialized");
        return;
    }

    Display* dpy = getXDisplay();

    if(!dpy)
    {
        throw std::runtime_error("x11 App was not correctly initialized");
        return;
    }

    xScreenNumber_ = ac->getXDefaultScreenNumber(); //todo: implement correctly

    unsigned long hints = win.getWindowHints();

    bool gl = 0;

    //renderer - nothing available
    #if (!defined NY_WithGL && !defined NY_WithCairo)
    throw std::runtime_error("x11WC::x11WC: no renderer available");
    return;
    #endif

    //WithGL
    #if (!defined NY_WithGL)
    if(settings.glPref == preference::Must)
    {
        throw std::runtime_error("x11WC::x11WC: no gl renderer available");
        return;
    }
    #else
    gl = 1;

    #endif

    //WithCairo
    #if (!defined NY_WithCairo)
    if(settings.glPref == preference::MustNot)
    {
        throw std::runtime_error("x11WC::x11WC: no software renderer available");
        return;
    }
    #else
    if(settings.glPref == preference::MustNot || settings.glPref == preference::ShouldNot)
        gl = 0;

    #endif


    //window type
    Window xParent;
    if(hints & windowHints::Toplevel)
    {
        windowType_ = x11WindowType::toplevel;

        toplevelWindow* tw = dynamic_cast<toplevelWindow*>(&win);
        if(!tw)
        {
            throw std::runtime_error("x11WC::x11WC: window has toplevel hint, but isnt a toplevel window");
            return;
        }

        if(gl)
            matchGLXVisualInfo();
        else
            matchVisualInfo();

        xParent = DefaultRootWindow(getXDisplay());

    }
    else if(hints & windowHints::Child)
    {
        windowType_ = x11WindowType::child;

        childWindow* cw = dynamic_cast<childWindow*>(&win);
        if(!cw)
        {
            throw std::runtime_error("x11WC::x11WC: window has child hint, but isnt a child window");
            return;
        }

        x11WC* parentWC = asX11(cw->getParent()->getWC());
        if(!parentWC || !(xParent = parentWC->getXWindow()))
        {
            throw std::runtime_error("x11WC::x11WC: could not find xParent");
            return;
        }

        if(gl)
        {
            if(parentWC->getDrawType() == x11DrawType::glx) //can take visual info from parent
                xVinfo_ = parentWC->getXVinfo();
            else
                matchGLXVisualInfo();
        }
        else
        {
            if(parentWC->getDrawType() == x11DrawType::cairo)
                xVinfo_ = parentWC->getXVinfo();
            else
                matchVisualInfo();
        }

    }

    unsigned int mask = CWColormap | CWEventMask;

    XSetWindowAttributes attr;
    attr.colormap = XCreateColormap(getXDisplay(), DefaultRootWindow(getXDisplay()), xVinfo_->visual, AllocNone);
    attr.event_mask = ExposureMask | StructureNotifyMask | MotionNotify | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask;

    if(!gl)
    {
        mask |= CWBackPixel | CWBorderPixel;
        attr.background_pixel = 0;
        attr.border_pixel = 0;
    }

    xWindow_ = XCreateWindow(getXDisplay(), xParent, win.getPositionX(), win.getPositionY(), win.getWidth(), win.getHeight(), 0, xVinfo_->depth, InputOutput, xVinfo_->visual, mask, &attr);

    ac->registerContext(xWindow_, this);
    if(hints & windowHints::Toplevel) XSetWMProtocols(getXDisplay(), xWindow_, &x11::WindowDelete, 1);

    if(gl)
    {
        //todo: egl
        drawType_ = x11DrawType::glx;
        glx_ = new glxDrawContext(*this);
    }
    else
    {
        drawType_ = x11DrawType::cairo;
        cairo_ = new x11CairoDrawContext(*this);
    }
}

x11WindowContext::~x11WindowContext()
{
    //todo: unregister

    if(drawType_ == x11DrawType::cairo && cairo_)
    {
        delete cairo_;
    }
    else if(drawType_ == x11DrawType::egl && egl_)
    {
        delete egl_;
    }
    else if(drawType_ == x11DrawType::glx)
    {
        if(glx_) delete glx_;
        if(glxFBC_) delete glxFBC_;
    }

    getX11AppContext()->unregisterContext(xWindow_);
    XDestroyWindow(getXDisplay(), xWindow_);

    if(ownedXVinfo_ && xVinfo_) delete xVinfo_;
}

void x11WindowContext::matchVisualInfo()
{
    if(!xVinfo_)
    {
        xVinfo_ = new XVisualInfo;
        ownedXVinfo_ = 1;
    }

    //todo: other visuals possible
    if(!XMatchVisualInfo(getXDisplay(), getX11AC()->getXDefaultScreenNumber(), 32, TrueColor, xVinfo_))
    {
        throw std::runtime_error("cant match X Visual info");
        return;
    }
}

void x11WindowContext::matchGLXVisualInfo()
{
    const int attribs[] =
    {
      GLX_X_RENDERABLE    , True,
      GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
      GLX_RENDER_TYPE     , GLX_RGBA_BIT,
      GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
      GLX_RED_SIZE        , 8,
      GLX_GREEN_SIZE      , 8,
      GLX_BLUE_SIZE       , 8,
      GLX_ALPHA_SIZE      , 8,
      GLX_DEPTH_SIZE      , 24,
      GLX_STENCIL_SIZE    , 8,
      GLX_DOUBLEBUFFER    , True,
      //GLX_SAMPLE_BUFFERS  , 1,
      //GLX_SAMPLES         , 4,
      None
    };


    int glxMajor, glxMinor;
    if (!glXQueryVersion(getXDisplay(), &glxMajor, &glxMinor) || ((glxMajor == 1) && (glxMinor < 3) ) || (glxMajor < 1)) //glx must be > 1.3
    {
        throw std::runtime_error("Invalid glx version. glx Version must be > 1.3");
        return;
    }

    int fbcount = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(getXDisplay(), DefaultScreen(getXDisplay()), attribs, &fbcount);
    if (!fbc || !fbcount)
    {
        throw std::runtime_error("failed to retrieve fbconfig");
        return;
    }

    //get the config with the most samples
    int best_fbc = -1, worst_fbc = -1, best_num_samp = 0, worst_num_samp = 0;
    for(int i(0); i < fbcount; i++)
    {
        XVisualInfo *vi = glXGetVisualFromFBConfig(getXDisplay(), fbc[i]);

        if(!vi)
        {
            XFree(vi);
            continue;
        }

        int samp_buf, samples;
        glXGetFBConfigAttrib(getXDisplay(), fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
        glXGetFBConfigAttrib(getXDisplay(), fbc[i], GLX_SAMPLES       , &samples  );

        if ( best_fbc < 0 || (samp_buf && samples > best_num_samp))
        {
            best_fbc = i;
            best_num_samp = samples;
        }

        if ( worst_fbc < 0 || (!samp_buf || samples < worst_num_samp))
        {
            worst_fbc = i;
            worst_num_samp = samples;
        }

        XFree(vi);
    }

    glxFBC_ = new glxFBC;
    glxFBC_->config = fbc[best_fbc];
    XFree(fbc);

    xVinfo_ = glXGetVisualFromFBConfig(getXDisplay(), glxFBC_->config);
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

    XSendEvent(getXDisplay(), xWindow_, False, ExposureMask, &ev);
    XFlush(getXDisplay());
}

drawContext& x11WindowContext::beginDraw()
{
    if(getCairo())
    {
        return *cairo_;
    }
    else if(getGLX())
    {
        return *glx_;
    }
    else
    {
        throw std::runtime_error("x11WC::beginDraw: no valid draw context");
    }
}

void x11WindowContext::finishDraw()
{
    if(getCairo())
    {
        cairo_->apply();
    }
    else if(getGLX())
    {
        glx_->apply();
        glx_->swapBuffers();
    }

    XFlush(getXDisplay());
}

void x11WindowContext::show()
{
    XMapWindow(getXDisplay(), xWindow_);
}

void x11WindowContext::hide()
{
    XUnmapWindow(getXDisplay(), xWindow_);
}

void x11WindowContext::raise()
{
    XRaiseWindow(getXDisplay(), xWindow_);
}

void x11WindowContext::lower()
{
    XLowerWindow(getXDisplay(), xWindow_);
}

void x11WindowContext::requestFocus()
{
    addState(x11::StateFocused); //todo: fix
}

void x11WindowContext::setSize(vec2ui size, bool change)
{
    if(change)
    {
        XResizeWindow(getXDisplay(), xWindow_, size.x, size.y);
    }

    if(getCairo())
        cairo_->setSize(size);
    else if(getGLX())
        glx_->setSize(size);

    refresh();

}

void x11WindowContext::setPosition(vec2i position, bool change)
{
    if(change)
    {
        XMoveWindow(getXDisplay(), xWindow_, position.x, position.y);
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
    XGetWMNormalHints(getXDisplay(), xWindow_, &s, &a);
    s.min_width = size.x;
    s.min_height = size.y;
    s.flags |= PMinSize;
    XSetWMNormalHints(getXDisplay(), xWindow_, &s);
}

void x11WindowContext::setMaxSize(vec2ui size)
{
    long a;
    XSizeHints s;
    XGetWMNormalHints(getXDisplay(), xWindow_, &s, &a);
    s.max_width = size.x;
    s.max_height = size.y;
    s.flags |= PMaxSize;
    XSetWMNormalHints(getXDisplay(), xWindow_, &s);
}

void x11WindowContext::sendContextEvent(contextEvent& e)
{
    if(e.contextEventType == X11Reparent)
    {
        wasReparented(e.to<x11ReparentEvent>());
    }
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

    XSendEvent(getXDisplay(), DefaultRootWindow(getXDisplay()), False, SubstructureNotifyMask, &ev);

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

    XSendEvent(getXDisplay(), DefaultRootWindow(getXDisplay()), False, SubstructureNotifyMask, &ev);

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

    XSendEvent(getXDisplay(), DefaultRootWindow(getXDisplay()), False, SubstructureNotifyMask, &ev);

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
    XChangeProperty(getXDisplay(), xWindow_, x11::MwmHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)&mhints, sizeof (x11::mwmHints)/sizeof (long));
}

void x11WindowContext::setMwmDecorationHints(const unsigned long hints)
{
    mwmDecoHints_ = hints;

    struct x11::mwmHints mhints;
    mhints.flags = x11::MwmHintsDeco;
    mhints.decorations = hints;
    XChangeProperty(getXDisplay(), xWindow_, x11::MwmHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)&mhints, sizeof (x11::mwmHints)/sizeof (long));
}

void x11WindowContext::setMwmFunctionHints(const unsigned long hints)
{
    mwmFuncHints_ = hints;

    struct x11::mwmHints mhints;
    mhints.flags = x11::MwmHintsFunc;
    mhints.functions = hints;
    XChangeProperty(getXDisplay(), xWindow_, x11::MwmHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)&mhints, sizeof (x11::mwmHints)/sizeof (long));
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

    XSendEvent(getXDisplay(), DefaultRootWindow(getXDisplay()), False, SubstructureNotifyMask, &ev);
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

    XSendEvent(getXDisplay(), DefaultRootWindow(getXDisplay()), False, SubstructureNotifyMask, &ev);
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

    XSetTransientForHint(getXDisplay(), other->getXWindow(), xWindow_);
}

void x11WindowContext::setType(const Atom type)
{
    XChangeProperty(getXDisplay(), xWindow_, x11::Type, XA_ATOM, 32, PropModeReplace, (unsigned char*) &type, 1);
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

    XChangeWindowAttributes(getXDisplay(), xWindow_, CWOverrideRedirect, &attr);
}

void x11WindowContext::setCursor(unsigned int xCursorID)
{
    Cursor c = XCreateFontCursor(getXDisplay(), xCursorID);
    XDefineCursor(getXDisplay(), xWindow_, c);
}

void x11WindowContext::wasReparented(x11ReparentEvent& ev)
{
    setPosition(window_.getPosition()); //set position correctly
}

void x11WindowContext::setMaximized()
{
    addState(x11::StateMaxHorz);
    addState(x11::StateMaxVert);
}

void x11WindowContext::setMinimized()
{
    XWMHints hints;
    hints.flags = StateHint;
    hints.initial_state = IconicState;
    XSetWMHints(getXDisplay(), xWindow_, &hints);
}

void x11WindowContext::setFullscreen()
{
    addState(x11::StateFullscreen);
}

void x11WindowContext::setNormal()
{
    XWMHints hints;
    hints.flags = StateHint;
    hints.initial_state = NormalState;
    XSetWMHints(getXDisplay(), xWindow_, &hints);
}

void x11WindowContext::beginMove(mouseButtonEvent* ev)
{
    x11EventData* xbev = dynamic_cast<x11EventData*>(ev->data);
    if(!xbev)
        return;

    XEvent xev = xbev->ev;

    XEvent mev;
    XUngrabPointer(getXDisplay(), 0L);

    mev.type = ClientMessage;
    mev.xclient.window = xWindow_;
    mev.xclient.message_type = x11::MoveResize;
    mev.xclient.format = 32;
    mev.xclient.data.l[0] = xev.xbutton.x_root;
    mev.xclient.data.l[1] = xev.xbutton.y_root;
    mev.xclient.data.l[2] = x11::MoveResizeMove;
    mev.xclient.data.l[3] = xev.xbutton.button;
    mev.xclient.data.l[4] = 1; //default. could be set to 2 for pager

    XSendEvent(getXDisplay(), DefaultRootWindow(getXDisplay()), False, SubstructureNotifyMask , &mev);
}

void x11WindowContext::beginResize(mouseButtonEvent* ev, windowEdge edge)
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
    XUngrabPointer(getXDisplay(), 0L);

    mev.type = ClientMessage;
    mev.xclient.window = xWindow_;
    mev.xclient.message_type = x11::MoveResize;
    mev.xclient.format = 32;
    mev.xclient.data.l[0] = xev.xbutton.x_root;
    mev.xclient.data.l[1] = xev.xbutton.y_root;
    mev.xclient.data.l[2] = x11Edge;
    mev.xclient.data.l[3] = xev.xbutton.button;
    mev.xclient.data.l[4] = 1; //default. could be set to 2 for pager

    XSendEvent(getXDisplay(), DefaultRootWindow(getXDisplay()), False, SubstructureNotifyMask , &mev);
}

void x11WindowContext::setIcon(const image* img)
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

void x11WindowContext::setName(std::string name)
{

}

/*
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

    xWindow_ = XCreateWindow(getXDisplay(), DefaultRootWindow(getXDisplay()), window_.getPositionX(), window_.getPositionY(), window_.getWidth(), window_.getHeight(), getToplevelWindow().getBorderSize(), xVinfo_->depth, winType, xVinfo_->visual, attrMask, &attr);

    //XClassHint hint;
    //hint.res_class = (char*) getToplevelWindow().getName().c_str();
    //hint.res_name = (char*) getToplevelWindow().getName().c_str();
    //XSetClassHint(getXDisplay(), xWindow_, &hint);

    XStoreName(getXDisplay(), xWindow_, getToplevelWindow().getName().c_str());

    XSetWMProtocols(getXDisplay(), xWindow_, &x11::WindowDelete, 1);
    context_->registerContext(xWindow_, this); //like user data for window
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

    xWindow_ = XCreateWindow(getXDisplay(), asX11(window_.getParent()->getWindowContext())->getXWindow(), window_.getPositionX(), window_.getPositionY(), window_.getWidth(), window_.getHeight(), 0, xVinfo_->depth, winType, xVinfo_->visual, attrMask, &attr);

    context_->registerContext(xWindow_, this); //like user data for window
}
*/

}
