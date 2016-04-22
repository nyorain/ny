#include <ny/backend/x11/windowContext.hpp>

#include <ny/backend/x11/util.hpp>
#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/internal.hpp>

#include <ny/app/app.hpp>
#include <ny/window/window.hpp>
#include <ny/window/child.hpp>
#include <ny/window/toplevel.hpp>
#include <ny/base/event.hpp>
#include <ny/base/log.hpp>
#include <ny/window/cursor.hpp>
#include <ny/window/events.hpp>
#include <ny/draw/image.hpp>
#include <ny/draw/drawContext.hpp>

#include <X11/Xatom.h>
#include <xcb/xcb_icccm.h>

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

	if(!xVisualID_) initVisual();

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

    xcb_window_t xparent = settings.parent;
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

	//show(); //???

    appContext_->registerContext(xWindow_, *this);
    if(toplvl) 
	{
		auto protocols = ewmhConnection()->WM_PROTOCOLS;
		auto list = appContext_->atom("WM_DELETE_WINDOW");

		xcb_change_property(xconn, XCB_PROP_MODE_REPLACE, xWindow_, protocols, 
				XCB_ATOM_ATOM, 32, 1, &list);
		xcb_change_property(xconn, XCB_PROP_MODE_REPLACE, xWindow_, XCB_ATOM_WM_NAME, 
				XCB_ATOM_STRING, 8, settings.title.size(), settings.title.c_str());
	}

    xcb_flush(xConnection());
}

X11WindowContext::~X11WindowContext()
{
    appContext().unregisterContext(xWindow_);

    xcb_destroy_window(xConnection(), xWindow_);
    xcb_flush(xConnection());
}

void X11WindowContext::initVisual()
{
	xVisualID_ = appContext().xDefaultScreen()->root_visual;
}

xcb_connection_t* X11WindowContext::xConnection() const
{
	return appContext().xConnection();
}
dummy_xcb_ewmh_connection_t* X11WindowContext::ewmhConnection() const
{
	return appContext().ewmhConnection();
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
   //nyMainApp()->dispatch(std::make_unique<DrawEvent>(eventHandler()));
   
	//x11 method
    xcb_expose_event_t ev{};

    ev.response_type = XCB_EXPOSE;
    ev.window = xWindow();

	xcb_send_event(xConnection(), 0, xWindow(), XCB_EVENT_MASK_EXPOSURE, (const char*)&ev);
	xcb_flush(xConnection());
}

void X11WindowContext::show()
{
    xcb_map_window(xConnection(), xWindow_);
	refresh();
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

void X11WindowContext::maximize()
{
    addState(ewmhConnection()->_NET_WM_STATE_MAXIMIZED_VERT,
			ewmhConnection()->_NET_WM_STATE_MAXIMIZED_HORZ);
}

void X11WindowContext::minimize()
{
	xcb_icccm_wm_hints_t hints;
    hints.flags = XCB_ICCCM_WM_HINT_STATE;
    hints.initial_state = XCB_ICCCM_WM_STATE_ICONIC;
    xcb_icccm_set_wm_hints(xConnection(), xWindow_, &hints);
}

void X11WindowContext::fullscreen()
{
    addState(ewmhConnection()->_NET_WM_STATE_FULLSCREEN);
}

void X11WindowContext::normalState()
{
	xcb_icccm_wm_hints_t hints;
    hints.flags = XCB_ICCCM_WM_HINT_STATE;
    hints.initial_state = XCB_ICCCM_WM_STATE_NORMAL;
    xcb_icccm_set_wm_hints(xConnection(), xWindow_, &hints);
}

void X11WindowContext::beginMove(const MouseButtonEvent* ev)
{
	auto* xbev = dynamic_cast<X11EventData*>(ev->data.get());
    if(!xbev) return;
    auto& xev = reinterpret_cast<xcb_button_press_event_t&>(xbev->event);

	//XXX: correct mouse button (index)!
	xcb_ewmh_request_wm_moveresize(ewmhConnection(), 0, xWindow(), xev.root_x, xev.root_y, 
		XCB_EWMH_WM_MOVERESIZE_MOVE, XCB_BUTTON_INDEX_1, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL); 
}

void X11WindowContext::beginResize(const MouseButtonEvent* ev, WindowEdges edge)
{
	auto* xbev = dynamic_cast<X11EventData*>(ev->data.get());
    if(!xbev) return;
    auto& xev = reinterpret_cast<xcb_button_press_event_t&>(xbev->event);

	xcb_ewmh_moveresize_direction_t x11Edge;
	switch(edge)
    {
        case WindowEdges::top: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_TOP; break;
        case WindowEdges::left: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_LEFT; break;
        case WindowEdges::bottom: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOM; break;
        case WindowEdges::right: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_RIGHT; break;
        case WindowEdges::topLeft: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_TOPLEFT; break;
        case WindowEdges::topRight: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_TOPRIGHT; break;
        case WindowEdges::bottomLeft: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOMLEFT; break;
        case WindowEdges::bottomRight: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOMRIGHT; break;
        default: return;
    }

	//XXX: correct mouse button!
	xcb_ewmh_request_wm_moveresize(ewmhConnection(), 0, xWindow(), xev.root_x, xev.root_y, 
		x11Edge, XCB_BUTTON_INDEX_1, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL); 
}

void X11WindowContext::icon(const Image* img)
{
    if(img)
    {
		auto cpy = *img;
		cpy.format(Image::Format::rgba8888);
		auto size = 2 + cpy.size().x * cpy.size().y;
		auto data = reinterpret_cast<std::uint32_t*>(cpy.data());
		xcb_ewmh_set_wm_icon(ewmhConnection(), XCB_PROP_MODE_REPLACE, xWindow(), size, data);
    }
	else
	{
		std::uint32_t buffer[2] = {0};
		xcb_ewmh_set_wm_icon(ewmhConnection(), XCB_PROP_MODE_REPLACE, xWindow(), 2, buffer);
	}
}

void X11WindowContext::title(const std::string& str)
{
	xcb_ewmh_set_wm_name(ewmhConnection(), xWindow(), str.size(), str.c_str());
}

NativeWindowHandle X11WindowContext::nativeHandle() const
{
	return NativeWindowHandle(xWindow_);
}

void X11WindowContext::minSize(const Vec2ui& size)
{
	xcb_size_hints_t hints {};
	hints.min_width = size.x;
	hints.min_height = size.y;
	hints.flags = XCB_ICCCM_SIZE_HINT_P_MIN_SIZE;
	xcb_icccm_set_wm_normal_hints(xConnection(), xWindow(), &hints);
}

void X11WindowContext::maxSize(const Vec2ui& size)
{
	xcb_size_hints_t hints {};
	hints.max_width = size.x;
	hints.max_height = size.y;
	hints.flags = XCB_ICCCM_SIZE_HINT_P_MAX_SIZE;
	xcb_icccm_set_wm_normal_hints(xConnection(), xWindow(), &hints);
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

	/*
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
	*/
}
void X11WindowContext::removeWindowHints(WindowHints hints)
{
    unsigned long motifDeco = 0;
    unsigned long motifFunc = 0;

	/*
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
	*/
}

//x11 specific
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
    addState(appContext().atom("_NET_WM_STATE_FOCUSED")); //todo: fix
}
void X11WindowContext::addState(xcb_atom_t state1, xcb_atom_t state2)
{
	xcb_ewmh_request_change_wm_state(ewmhConnection(), 0, xWindow(), XCB_EWMH_WM_STATE_ADD, 
			state1, state2, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL);
}

void X11WindowContext::removeState(xcb_atom_t state1, xcb_atom_t state2)
{
	xcb_ewmh_request_change_wm_state(ewmhConnection(), 0, xWindow(), XCB_EWMH_WM_STATE_REMOVE, 
			state1, state2, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL);
}

void X11WindowContext::toggleState(xcb_atom_t state1, xcb_atom_t state2)
{
	xcb_ewmh_request_change_wm_state(ewmhConnection(), 0, xWindow(), XCB_EWMH_WM_STATE_TOGGLE, 
			state1, state2, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL);
}

void X11WindowContext::mwmHints(unsigned long deco, unsigned long func, bool d, bool f)
{
	/*
    struct x11::MwmHints mhints;
    if(d) 
	{
		mwmDecoHints_ = deco;
		mhints.flags |= x11::MwmHintsDeco;
		mhints.decorations = deco;
	}
    if(f) 
	{
		mwmFuncHints_ = func;
		mhints.flags |= x11::MwmHintsFunc;
		mhints.functions = func;
	}

    xcb_change_property(xConnection(), xWindow(), appContext().atom("_MOTIF_WM_HINTS"), XA_ATOM, 
			32, PropModeReplace, (unsigned char *)&mhints, sizeof (x11::MwmHints)/sizeof (long));
			*/
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
	/*
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = xWindow_;
    ev.xclient.message_type = x11::AllowedActions;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 1; //add
    ev.xclient.data.l[1] = action;
    ev.xclient.data.l[2] = 0;

    XSendEvent(xDisplay(), DefaultRootWindow(xDisplay()), False, SubstructureNotifyMask, &ev);
	*/
}

void X11WindowContext::removeAllowedAction(xcb_atom_t action)
{
	/*
    XEvent ev;

    ev.type = ClientMessage;
    ev.xclient.window = xWindow_;
    ev.xclient.message_type = x11::AllowedActions;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 0; //remove
    ev.xclient.data.l[1] = action;
    ev.xclient.data.l[2] = 0;

    XSendEvent(xDisplay(), DefaultRootWindow(xDisplay()), False, SubstructureNotifyMask, &ev);
	*/
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
//    XSetTransientForHint(xDisplay(), other, xWindow_);
}

void X11WindowContext::xWindowType(xcb_window_t type)
{
//    XChangeProperty(xDisplay(), xWindow_, x11::Type, XA_ATOM, 32, PropModeReplace, (unsigned char*) &type, 1);
}

xcb_atom_t X11WindowContext::xWindowType()
{
    return 0;
    //todo
}

void X11WindowContext::overrideRedirect(bool redirect)
{
/*
    XSetWindowAttributes attr;
    attr.override_rediRect = rediRect;

    XChangeWindowAttributes(xDisplay(), xWindow_, CWOverrideRedirect, &attr);
*/
}

void X11WindowContext::cursor(unsigned int xCursorID)
{
/*
    XCursor c = XCreateFontCursor(xDisplay(), xCursorID);
    XDefineCursor(xDisplay(), xWindow_, c);
*/
}

}
