#include <ny/backend/x11/windowContext.hpp>

#include <ny/backend/x11/util.hpp>
#include <ny/backend/x11/appContext.hpp>

#include <ny/app/app.hpp>
#include <ny/window/window.hpp>
#include <ny/window/child.hpp>
#include <ny/window/toplevel.hpp>
#include <ny/base/event.hpp>
#include <ny/window/cursor.hpp>
#include <ny/window/events.hpp>
#include <ny/draw/image.hpp>

#include <X11/Xatom.h>

#include <memory.h>
#include <iostream>

namespace ny
{

/*
bool usingGLX(Preference glPref)
{
    //renderer - nothing available
    #if (!defined NY_WithGL && !defined NY_WithCairo)
     throw std::runtime_error("x11WC::x11WC: no renderer available");
    #endif

    //WithGL
    #if (!defined NY_WithGL)
     if(glPref == Preference::must) 
		throw std::runtime_error("x11WC::x11WC: no gl renderer available, preference is must");
     else return 0;

    #else
     if(glPref == Preference::must || glPref == Preference::should) return 1;
    #endif

    //WithCairo
    #if (!defined NY_WithCairo)
     if(glPref == Preference::mustNot) 
		throw std::runtime_error("x11WC::x11WC: no software renderer available");
     else return 1;

    #else
     if(glPref == Preference::mustNot || glPref == Preference::shouldNot) return 0;

    #endif

	return 0;
}
*/

//windowContext
X11WindowContext::X11WindowContext(X11AppContext& ctx, const X11WindowSettings& settings) 
	: appContext_(&ctx)
{
    auto xconn = appContext_->xConnection();
	auto xscreen = appContext_->xDefaultScreen();
    if(!xconn || !xscreen)
    {
        throw std::runtime_error("ny::X11WC: x11 App was not corRectly initialized");
        return;
    }

    //window type
	bool toplvl = 0;
	auto pos = settings.position;
	auto size = settings.size;

    xcb_window_t xparent = settings.nativeHandle;
	if(!xparent)
	{
		xparent = xscreen->root;
		toplvl = 1;
	}

	xcb_colormap_t colormap = xcb_generate_id(xconn);
	xcb_create_colormap(xconn, XCB_COLORMAP_ALLOC_NONE, colormap, xscreen->root, xVisualID_);

	std::uint32_t eventmask = 
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS |
		XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION;

	std::uint32_t valuelist[] = {eventmask, colormap, 0};
	std::uint32_t valuemask = XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;

	xWindow_ = xcb_generate_id(xconn);
	xcb_create_window(xconn, XCB_COPY_FROM_PARENT, xWindow_, xparent, pos.x, pos.y,
		size.x, size.y, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, xVisualID_, valuemask, valuelist);

    appContext_->registerContext(xWindow_, *this);
    if(toplvl) 
	{
		auto protocols = appContext_->atom("WM_WINDOW_PROTOCOLS");
		auto list = appContext_->atom("WM_WINDOW_DELETE");

		xcb_change_property(xconn, XCB_PROP_MODE_REPLACE, xWindow_, protocols, 
				XCB_ATOM_ATOM, 32, 1, &list);
		xcb_change_property(xconn, XCB_PROP_MODE_REPLACE, xWindow_, XCB_ATOM_WM_NAME, 
				XCB_ATOM_STRING, 8, settings.title.size(), settings.title.c_str());
	}

	//make it transparent (even with opengl)
	/*
	auto alpha = 0.2;
	auto cardinal_alpha = (uint32_t) (alpha * (uint32_t) -1);

	if (cardinal_alpha == (uint32_t)-1) 
	{
		XDeleteProperty(xDisplay(), xWindow_, XInternAtom(xDisplay(), "_NET_WM_WINDOW_OPACITY", 0)) ;
	} 
	else 
	{
		XChangeProperty(xDisplay(), xWindow_, XInternAtom(xDisplay(), "_NET_WM_WINDOW_OPACITY", 0),
			XA_CARDINAL, 32, PropModeReplace, (uint8_t*) &cardinal_alpha, 1) ;
	}
   */
}

void X11WindowContext::create()
{
}

X11WindowContext::~X11WindowContext()
{
	if(cairo()) cairo_.reset();
	else if(glx()) glx_.reset();

    if(ownedXVinfo_ && xVinfo_) delete xVinfo_;
    x11AppContext()->unregisterContext(xWindow_);

    XDestroyWindow(xDisplay(), xWindow_);
    XFlush(xDisplay());

}

void X11WindowContext::matchVisualInfo()
{
    if(!xVinfo_)
    {
        xVinfo_ = new XVisualInfo;
        ownedXVinfo_ = 1;
    }

    //todo: other visuals possible
    if(!XMatchVisualInfo(xDisplay(), x11AppContext()->xDefaultScreenNumber(), 32, 
				TrueColor, xVinfo_))
    {
        throw std::runtime_error("cant match X Visual info");
        return;
    }
}

GLXFBConfig X11WindowContext::matchGLXVisualInfo()
{
#ifdef NY_WithGL	
    const int attribs[] =
    {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        None
    };

    int glxMajor, glxMinor;
    if(!glXQueryVersion(xDisplay(), &glxMajor, &glxMinor) 
			|| ((glxMajor == 1) && (glxMinor < 3) ) || (glxMajor < 1))
    {
        throw std::runtime_error("Invalid glx version. glx Version must be > 1.3");
    }

    int fbcount = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(xDisplay(), DefaultScreen(xDisplay()), attribs, &fbcount);
    if (!fbc || !fbcount)
    {
        throw std::runtime_error("failed to retrieve fbconfig");
    }

    //get the config with the most samples
    int best_fbc = -1, worst_fbc = -1, best_num_samp = 0, worst_num_samp = 0;
    for(int i(0); i < fbcount; i++)
    {
        XVisualInfo *vi = glXGetVisualFromFBConfig(xDisplay(), fbc[i]);

        if(!vi) continue;

        int samp_buf, samples;
        glXGetFBConfigAttrib(xDisplay(), fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
        glXGetFBConfigAttrib(xDisplay(), fbc[i], GLX_SAMPLES, &samples);

        if(best_fbc < 0 || (samp_buf && samples > best_num_samp))
        {
            best_fbc = i;
            best_num_samp = samples;
        }

        if(worst_fbc < 0 || (!samp_buf || samples < worst_num_samp))
        {
            worst_fbc = i;
            worst_num_samp = samples;
        }

        XFree(vi);
    }

	auto ret = fbc[best_fbc];
    XFree(fbc);

    xVinfo_ = glXGetVisualFromFBConfig(xDisplay(), ret);
	return ret;
#endif
}


void X11WindowContext::refresh()
{
    nyMainApp()->dispatch(make_unique<DrawEvent>(&window()));

/*
    //x11 method
    XEvent ev{};

    ev.type = Expose;
    ev.xexpose.window = xWindow_;

    XSendEvent(xDisplay(), xWindow_, False, ExposureMask, &ev);
    XFlush(xDisplay());
*/
}

DrawContext& X11WindowContext::beginDraw()
{
    if(cairo())
    {
        return *cairo_;
    }
    else if(glx())
    {
		#ifdef NY_WithGL
         glx_->makeCurrent();
		 auto& ret = glx_->drawContext();
		 ret.viewport(Rect2f({0.f, 0.f}, window().size()));
		 return ret;
		#endif
    }

	throw std::runtime_error("X11WC:beginDraw on invalid x11WC");
}

void X11WindowContext::finishDraw()
{
    if(cairo())
    {
        cairo_->apply();
    }
    else if(glx())
    {
		#ifdef NY_WithGL
         glx_->apply();
         glx_->makeNotCurrent();
		#endif
    }

    XFlush(xDisplay());
}

void X11WindowContext::show()
{
    XMapWindow(xDisplay(), xWindow_);
}

void X11WindowContext::hide()
{
    XUnmapWindow(xDisplay(), xWindow_);
}

void X11WindowContext::raise()
{
    XRaiseWindow(xDisplay(), xWindow_);
}

void X11WindowContext::lower()
{
    XLowerWindow(xDisplay(), xWindow_);
}

void X11WindowContext::requestFocus()
{
    addState(x11::StateFocused); //todo: fix
}

void X11WindowContext::size(const Vec2ui& size, bool change)
{
    if(change) XResizeWindow(xDisplay(), xWindow_, size.x, size.y);

    if(cairo()) cairo_->size(size);

	#ifdef NY_WithGL
     else if(glx()) glx_->size(size);
	#endif

    refresh();
}

void X11WindowContext::position(const Vec2i& position, bool change)
{
    if(change) XMoveWindow(xDisplay(), xWindow_, position.x, position.y);
}

void X11WindowContext::cursor(const Cursor& curs)
{
    if(curs.isNativeType())
    {
        int num = cursorToX11(curs.nativeType());

        if(num != -1)
        {
            cursor(num);
        }
    }
    //todo: image
}

NativeWindowHandle X11WindowContext::nativeHandle() const
{
	return NativeWindowHandle(xWindow_);
}

void X11WindowContext::minSize(const Vec2ui& size)
{
    long a;
    XSizeHints s;
    XGetWMNormalHints(xDisplay(), xWindow_, &s, &a);
    s.min_width = size.x;
    s.min_height = size.y;
    s.flags |= PMinSize;
    XSetWMNormalHints(xDisplay(), xWindow_, &s);
}

void X11WindowContext::maxSize(const Vec2ui& size)
{
    long a;
    XSizeHints s;
    XGetWMNormalHints(xDisplay(), xWindow_, &s, &a);
    s.max_width = size.x;
    s.max_height = size.y;
    s.flags |= PMaxSize;
    XSetWMNormalHints(xDisplay(), xWindow_, &s);
}

void X11WindowContext::processEvent(const ContextEvent& e)
{
    if(e.contextType() == eventType::context::x11Reparent) 
	{
		reparented(static_cast<const X11ReparentEvent&>(e).event);
	}
}

void X11WindowContext::addWindowHints(unsigned long hints)
{
    unsigned long motifDeco = 0;
    unsigned long motifFunc = 0;
	bool customDecorated = 0;

	if(window().windowHints() & windowHints::customDecorated)
	{
		if(mwmDecoHints_ != 0)
		{
			mwmDecoHints_ = 0;
			mwmHints(mwmDecoHints_, mwmFuncHints_);
		}

		customDecorated = 1;
	}
    if(hints & windowHints::close)
    {
        motifFunc |= x11::MwmFuncClose;
        addAllowedAction(x11::AllowedActionClose);
    }
    if(hints & windowHints::maximize)
    {
        motifFunc |= x11::MwmFuncMaximize;
        motifDeco |= x11::MwmDecoMaximize;

        addAllowedAction(x11::AllowedActionMaximizeHorz);
        addAllowedAction(x11::AllowedActionMaximizeVert);
    }

    if(hints & windowHints::minimize)
    {
        motifFunc |= x11::MwmFuncMinimize;
        motifDeco |= x11::MwmDecoMinimize;

        addAllowedAction(x11::AllowedActionMinimize);
    }
    if(hints & windowHints::move)
    {
        motifFunc |= x11::MwmFuncMove;
        motifDeco |= x11::MwmDecoTitle;

        addAllowedAction(x11::AllowedActionMove);
    }
    if(hints & windowHints::resize)
    {
        motifFunc |= x11::MwmFuncResize;
        motifDeco |= x11::MwmDecoResize;

        addAllowedAction(x11::AllowedActionResize);
    }
    if(hints & windowHints::showInTaskbar)
    {
        removeState(x11::StateSkipPager);
        removeState(x11::StateSkipTaskbar);
    }
    if(hints & windowHints::alwaysOnTop)
    {
        addState(x11::StateAbove);
    }

	if(customDecorated)
	{
		motifDeco = 0;
	}

	if(motifFunc != 0 || motifDeco != 0)
	{
		mwmFuncHints_ |= motifFunc;
		mwmDecoHints_ |= motifDeco;
		mwmHints(mwmDecoHints_, mwmFuncHints_);
	}
}
void X11WindowContext::removeWindowHints(unsigned long hints)
{
    unsigned long motifDeco = 0;
    unsigned long motifFunc = 0;

    if(hints & windowHints::close)
    {
        motifFunc |= x11::MwmFuncClose;
        removeAllowedAction(x11::AllowedActionClose);
    }
    if(hints & windowHints::maximize)
    {
        motifFunc |= x11::MwmFuncMaximize;
        motifDeco |= x11::MwmDecoMaximize;

        removeAllowedAction(x11::AllowedActionMaximizeHorz);
        removeAllowedAction(x11::AllowedActionMaximizeVert);
    }

    if(hints & windowHints::minimize)
    {
        motifFunc |= x11::MwmFuncMinimize;
        motifDeco |= x11::MwmDecoMinimize;

        removeAllowedAction(x11::AllowedActionMinimize);
    }
    if(hints & windowHints::move)
    {
        motifFunc |= x11::MwmFuncMove;
        motifDeco |= x11::MwmDecoTitle;

        removeAllowedAction(x11::AllowedActionMove);
    }
    if(hints & windowHints::resize)
    {
        motifFunc |= x11::MwmFuncResize;
        motifDeco |= x11::MwmDecoResize;

        removeAllowedAction(x11::AllowedActionResize);
    }
    if(hints & windowHints::showInTaskbar)
    {
        addState(x11::StateSkipPager);
        addState(x11::StateSkipTaskbar);
    }
    if(hints & windowHints::alwaysOnTop)
    {
        removeState(x11::StateAbove);
    }

    mwmFuncHints_ &= ~motifFunc;
    mwmDecoHints_ &= ~motifDeco;

    mwmHints(mwmDecoHints_, mwmFuncHints_);
}

//x11 specific
void X11WindowContext::addState(Atom state)
{
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = xWindow_;
    ev.xclient.message_type = x11::State;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 1; //add
    ev.xclient.data.l[1] = state;
    ev.xclient.data.l[2] = 0;

    XSendEvent(xDisplay(), DefaultRootWindow(xDisplay()), False, SubstructureNotifyMask, &ev);

    states_ |= state;
}

void X11WindowContext::removeState(Atom state)
{
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = xWindow_;
    ev.xclient.message_type = x11::State;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 0; //remove
    ev.xclient.data.l[1] = state;
    ev.xclient.data.l[2] = 0;

    XSendEvent(xDisplay(), DefaultRootWindow(xDisplay()), False, SubstructureNotifyMask, &ev);

    states_ = states_ & ~state;
}

void X11WindowContext::toggleState(Atom state)
{
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = xWindow_;
    ev.xclient.message_type = x11::State;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 2; //toggle
    ev.xclient.data.l[1] = state;
    ev.xclient.data.l[2] = 0;

    XSendEvent(xDisplay(), DefaultRootWindow(xDisplay()), False, SubstructureNotifyMask, &ev);

    states_ = states_ xor state;
}

void X11WindowContext::mwmHints(unsigned long deco, unsigned long func)
{
    mwmDecoHints_ = deco;
    mwmFuncHints_ = func;

    struct x11::mwmHints mhints;
    mhints.flags = x11::MwmHintsDeco | x11::MwmHintsFunc;
    mhints.decorations = deco;
    mhints.functions = func;
    XChangeProperty(xDisplay(), xWindow_, x11::MwmHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)&mhints, sizeof (x11::mwmHints)/sizeof (long));
}

void X11WindowContext::mwmDecorationHints(const unsigned long hints)
{
    mwmDecoHints_ = hints;

    struct x11::mwmHints mhints;
    mhints.flags = x11::MwmHintsDeco;
    mhints.decorations = hints;
    XChangeProperty(xDisplay(), xWindow_, x11::MwmHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)&mhints, sizeof (x11::mwmHints)/sizeof (long));
}

void X11WindowContext::mwmFunctionHints(const unsigned long hints)
{
    mwmFuncHints_ = hints;

    struct x11::mwmHints mhints;
    mhints.flags = x11::MwmHintsFunc;
    mhints.functions = hints;
    XChangeProperty(xDisplay(), xWindow_, x11::MwmHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)&mhints, sizeof (x11::mwmHints)/sizeof (long));
}

unsigned long X11WindowContext::mwmFunctionHints() const
{
    //todo
    return 0;
}

unsigned long X11WindowContext::mwmDecorationHints() const
{
    //todo
    return 0;
}

void X11WindowContext::addAllowedAction(Atom action)
{
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = xWindow_;
    ev.xclient.message_type = x11::AllowedActions;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 1; //add
    ev.xclient.data.l[1] = action;
    ev.xclient.data.l[2] = 0;

    XSendEvent(xDisplay(), DefaultRootWindow(xDisplay()), False, SubstructureNotifyMask, &ev);
}

void X11WindowContext::removeAllowedAction(Atom action)
{
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = xWindow_;
    ev.xclient.message_type = x11::AllowedActions;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 0; //remove
    ev.xclient.data.l[1] = action;
    ev.xclient.data.l[2] = 0;

    XSendEvent(xDisplay(), DefaultRootWindow(xDisplay()), False, SubstructureNotifyMask, &ev);
}

std::vector<Atom> X11WindowContext::allowedActions() const
{
    std::vector<Atom> ret;
    return ret;

    //todo
}

void X11WindowContext::refreshStates()
{
    //todo
}

void X11WindowContext::transientFor(Window& w)
{
    auto* other = asX11(w.windowContext());
    XSetTransientForHint(xDisplay(), other->xWindow(), xWindow_);
}

void X11WindowContext::xWindowType(const Atom type)
{
    XChangeProperty(xDisplay(), xWindow_, x11::Type, XA_ATOM, 32, PropModeReplace, (unsigned char*) &type, 1);
}

Atom X11WindowContext::xWindowType()
{
    return 0;
    //todo
}

void X11WindowContext::overrideRedirect(bool rediRect)
{
    XSetWindowAttributes attr;
    attr.override_rediRect = rediRect;

    XChangeWindowAttributes(xDisplay(), xWindow_, CWOverrideRedirect, &attr);
}

void X11WindowContext::cursor(unsigned int xCursorID)
{
    XCursor c = XCreateFontCursor(xDisplay(), xCursorID);
    XDefineCursor(xDisplay(), xWindow_, c);
}

void X11WindowContext::reparented(const XReparentEvent&)
{
    position(window_->position()); //set position corRectly
}

void X11WindowContext::maximized()
{
    addState(x11::StateMaxHorz);
    addState(x11::StateMaxVert);
}

void X11WindowContext::minimized()
{
    XWMHints hints;
    hints.flags = StateHint;
    hints.initial_state = IconicState;
    XSetWMHints(xDisplay(), xWindow_, &hints);
}

void X11WindowContext::fullscreen()
{
    addState(x11::StateFullscreen);
}

void X11WindowContext::toplevel()
{
    XWMHints hints;
    hints.flags = StateHint;
    hints.initial_state = NormalState;
    XSetWMHints(xDisplay(), xWindow_, &hints);
}

void X11WindowContext::beginMove(const MouseButtonEvent* ev)
{
	auto* xbev = dynamic_cast<X11EventData*>(ev->data.get());
    if(!xbev)
        return;

    auto& xev = reinterpret_cast<xcb_button_press_event_t&>(xbev->event);

    XEvent mev;
    XUngrabPointer(xDisplay(), 0L);

    mev.type = ClientMessage;
    mev.xclient.window = xWindow_;
    mev.xclient.message_type = x11::MoveResize;
    mev.xclient.format = 32;
    mev.xclient.data.l[0] = xev.root_x;
    mev.xclient.data.l[1] = xev.root_y;
    mev.xclient.data.l[2] = x11::MoveResizeMove;
    mev.xclient.data.l[3] = xev.detail;
    mev.xclient.data.l[4] = 1; //default. could be set to 2 for pager

    XSendEvent(xDisplay(), DefaultRootWindow(xDisplay()), False, SubstructureNotifyMask , &mev);
}

void X11WindowContext::beginResize(const MouseButtonEvent* ev, WindowEdge edge)
{
    auto* xbev = dynamic_cast<X11EventData*>(ev->data.get());

    if(!xbev)
        return;

    unsigned long x11Edge = 0;

    switch(edge)
    {
        case WindowEdge::top: x11Edge = x11::MoveResizeSizeTop; break;
        case WindowEdge::left: x11Edge = x11::MoveResizeSizeLeft; break;
        case WindowEdge::bottom: x11Edge = x11::MoveResizeSizeBottom; break;
        case WindowEdge::right: x11Edge = x11::MoveResizeSizeRight; break;
        case WindowEdge::topLeft: x11Edge = x11::MoveResizeSizeTopLeft; break;
        case WindowEdge::topRight: x11Edge = x11::MoveResizeSizeTopRight; break;
        case WindowEdge::bottomLeft: x11Edge = x11::MoveResizeSizeBottomLeft; break;
        case WindowEdge::bottomRight: x11Edge = x11::MoveResizeSizeBottomRight; break;
        default: return;
    }

    auto& xev = reinterpret_cast<xcb_button_press_event_t&>(xbev->event);

    XEvent mev;
    XUngrabPointer(xDisplay(), 0L);

    mev.type = ClientMessage;
    mev.xclient.window = xWindow_;
    mev.xclient.message_type = x11::MoveResize;
    mev.xclient.format = 32;
    mev.xclient.data.l[0] = xev.root_x;
    mev.xclient.data.l[1] = xev.root_y;
    mev.xclient.data.l[2] = x11Edge;
    mev.xclient.data.l[3] = xev.detail;
    mev.xclient.data.l[4] = 1; //default. could be set to 2 for pager

    XSendEvent(xDisplay(), DefaultRootWindow(xDisplay()), False, SubstructureNotifyMask , &mev);
}

void X11WindowContext::icon(const Image* img)
{
    //TODO: only rgba images accepted atm
    if(img)
    {
        auto imageData = img->data();
        XChangeProperty(xDisplay(), xWindow_, x11::WMIcon, x11::Cardinal, 32, PropModeReplace, 
				imageData, 2 + img->size().x * img->size().y);

        return;
    }

	unsigned char buffer[8] = {0};
    XChangeProperty(xDisplay(), xWindow_, x11::WMIcon, x11::Cardinal, 32, 
			PropModeReplace, buffer, 2);
}

void X11WindowContext::title(const std::string&)
{

}

/*
//toplevel///
////////////
x11ToplevelWindowContext::x11ToplevelWindowContext(toplevelWindow& win, const X11WindowContextSettings& settings, bool pcreate) : windowContext(win, settings), toplevelWindowContext(win, settings), X11WindowContext(win, settings)
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

    xWindow_ = XCreateWindow(xDisplay(), DefaultRootWindow(xDisplay()), window_.getPositionX(), window_.getPositionY(), window_.getWidth(), window_.getHeight(), getToplevelWindow().getBorderSize(), xVinfo_->depth, winType, xVinfo_->visual, attrMask, &attr);

    //XClassHint hint;
    //hint.res_class = (char*) getToplevelWindow().getName().c_str();
    //hint.res_name = (char*) getToplevelWindow().getName().c_str();
    //XSetClassHint(xDisplay(), xWindow_, &hint);

    XStoreName(xDisplay(), xWindow_, getToplevelWindow().getName().c_str());

    XSetWMProtocols(xDisplay(), xWindow_, &x11::WindowDelete, 1);
    context_->registerContext(xWindow_, this); //like user data for window
}



//x11ChildWC////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////77
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
x11ChildWindowContext::x11ChildWindowContext(childWindow& win, const X11WindowContextSettings& settings, bool pcreate) : windowContext(win, settings), childWindowContext(win, settings), X11WindowContext(win, settings)
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

    xWindow_ = XCreateWindow(xDisplay(), asX11(window_.getParent()->getWindowContext())->getXWindow(), window_.getPositionX(), window_.getPositionY(), window_.getWidth(), window_.getHeight(), 0, xVinfo_->depth, winType, xVinfo_->visual, attrMask, &attr);

    context_->registerContext(xWindow_, this); //like user data for window
}
*/

}
