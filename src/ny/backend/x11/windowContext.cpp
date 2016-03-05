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
#include <ny/draw/drawContext.hpp>

#include <X11/Xatom.h>

#include <memory.h>
#include <iostream>

namespace ny
{

//windowContext
X11WindowContext::X11WindowContext(X11AppContext& ctx, const X11WindowSettings& settings) 
{
	create(ctx, settings);
}

void X11WindowContext::create(X11AppContext& ctx, const X11WindowSettings& settings) 
{
	appContext_ = &ctx;

    auto xconn = appContext_->xConnection();
	auto xscreen = appContext_->xDefaultScreen();
    if(!xconn || !xscreen)
    {
        throw std::runtime_error("ny::X11WC: x11 App was not correctly initialized");
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
}

X11WindowContext::~X11WindowContext()
{
    appContext().unregisterContext(xWindow_);

    xcb_destroy_window(xConnection(), xWindow_);
    xcb_flush(xConnection());
}

void X11WindowContext::initVisual()
{
}

/*
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
*/

DrawGuard X11WindowContext::draw()
{
	throw std::logic_error("ny::X11WC: called draw() on draw-less windowContext");
}

void X11WindowContext::refresh()
{
   // nyMainApp()->dispatch(make_unique<DrawEvent>(&window()));

/*
    //x11 method
    XEvent ev{};

    ev.type = Expose;
    ev.xexpose.window = xWindow_;

    XSendEvent(xDisplay(), xWindow_, False, ExposureMask, &ev);
    XFlush(xDisplay());
*/
}

void X11WindowContext::show()
{
    xcb_map_window(xConnection(), xWindow_);
}

void X11WindowContext::hide()
{
    xcb_unmap_window(xConnection(), xWindow_);
}

void X11WindowContext::raise()
{
}

void X11WindowContext::lower()
{
}

void X11WindowContext::requestFocus()
{
    addState(x11::StateFocused); //todo: fix
}

void X11WindowContext::size(const Vec2ui& size)
{
	xcb_configure_window(xConnection(), xWindow_, 
		XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, size.data());
    refresh();
}

void X11WindowContext::position(const Vec2i& position)
{
	auto data = reinterpret_cast<const unsigned int*>(position.data());
	xcb_configure_window(xConnection(), xWindow_, 
		XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, data);
    refresh();
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

bool X11WindowContext::handleEvent(const Event& e)
{
    if(e.type() == eventType::x11::reparent) 
	{
	}

	return 0;
}

bool X11WindowContext::customDecorated() const
{
	return 0;
}

void X11WindowContext::addWindowHints(WindowHints hints)
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
void X11WindowContext::removeWindowHints(WindowHints hints)
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
void X11WindowContext::addState(xcb_atom_t state)
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

void X11WindowContext::removeState(xcb_atom_t state)
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

void X11WindowContext::toggleState(xcb_atom_t state)
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

void X11WindowContext::addAllowedAction(xcb_atom_t action)
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

void X11WindowContext::removeAllowedAction(xcb_atom_t action)
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

std::vector<xcb_atom_t> X11WindowContext::allowedActions() const
{
    std::vector<xcb_atom_t> ret;
    return ret;

    //todo
}

void X11WindowContext::refreshStates()
{
    //todo
}

void X11WindowContext::transientFor(xcb_window_t other)
{
    XSetTransientForHint(xDisplay(), other, xWindow_);
}

void X11WindowContext::xWindowType(xcb_window_t type)
{
    XChangeProperty(xDisplay(), xWindow_, x11::Type, XA_ATOM, 32, PropModeReplace, (unsigned char*) &type, 1);
}

xcb_atom_t X11WindowContext::xWindowType()
{
    return 0;
    //todo
}

void X11WindowContext::overrideRedirect(bool redirect)
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

void X11WindowContext::maximize()
{
    addState(x11::StateMaxHorz);
    addState(x11::StateMaxVert);
}

void X11WindowContext::minimize()
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

}
