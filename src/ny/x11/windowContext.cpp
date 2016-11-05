#include <ny/x11/windowContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/x11/defs.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/surface.hpp>

#include <ny/common/unix.hpp>
#include <ny/events.hpp>
#include <ny/event.hpp>
#include <ny/log.hpp>
#include <ny/cursor.hpp>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_image.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xlib.h>

#include <cstring> //memcpy

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
	settings_ = settings;

	if(!visualID_) initVisual();

	auto visualtype = xVisualType();
	if(!visualtype) throw std::runtime_error("ny::X11WC: failed to retrieve the visualtype");
	auto vid = visualtype->visual_id;

    auto xconn = appContext_->xConnection();
	auto xscreen = appContext_->xDefaultScreen();
    if(!xconn || !xscreen) throw std::runtime_error("ny::X11WC: invalid X11AppContext");

	bool toplvl = false;
	auto pos = settings.position;
	auto size = settings.size;

    xcb_window_t xparent = settings.parent.uint64();
	if(!xparent)
	{
		xparent = xscreen->root;
		toplvl = true;
	}

	xcb_colormap_t colormap = xcb_generate_id(xconn);
	xcb_create_colormap(xconn, XCB_COLORMAP_ALLOC_NONE, colormap, xscreen->root, vid);

	std::uint32_t eventmask =
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS |
		XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION;

	std::uint32_t valuelist[] = {0, 0, eventmask, colormap, 0};
	std::uint32_t valuemask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | 
		XCB_CW_COLORMAP;

	xWindow_ = xcb_generate_id(xconn);
	auto cookie = xcb_create_window_checked(xconn, depth_, xWindow_, xparent, pos.x, pos.y,
		size.x, size.y, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, vid, valuemask, valuelist);

	if(!x11::testCookie(*xConnection(), cookie))
		throw std::runtime_error("ny::X11WC: Failed to create the x window.");

    appContext_->registerContext(xWindow_, *this);
    if(toplvl)
	{
		auto protocols = ewmhConnection()->WM_PROTOCOLS;
		auto list = appContext_->atoms().wmDeleteWindow;

		xcb_change_property(xconn, XCB_PROP_MODE_REPLACE, xWindow_, protocols,
				XCB_ATOM_ATOM, 32, 1, &list);
		xcb_change_property(xconn, XCB_PROP_MODE_REPLACE, xWindow_, XCB_ATOM_WM_NAME,
				XCB_ATOM_STRING, 8, settings.title.size(), settings.title.c_str());
	}

	cursor(settings.cursor);
	if(settings.show) show();
    xcb_flush(xConnection());
}

X11WindowContext::~X11WindowContext()
{
	if(xWindow_)
	{
		appContext().unregisterContext(xWindow_);
		xcb_destroy_window(xConnection(), xWindow_);
	}

	if(xCursor_) xcb_free_cursor(xConnection(), xCursor_);
	xcb_flush(xConnection());
}

void X11WindowContext::initVisual()
{
	visualID_ = 0u;
    auto screen = appContext().xDefaultScreen();
	auto avDepth = 0u;

	auto depth_iter = xcb_screen_allowed_depths_iterator(screen);
	for(; depth_iter.rem; xcb_depth_next(&depth_iter)) 
	{
		if(depth_iter.data->depth == 32) avDepth = 32;
		if(!avDepth && depth_iter.data->depth == 24) avDepth = 24;
	}

	if(avDepth == 0u) throw std::runtime_error("X11WC: no 24 or 32 bit visuals.");
	else if(avDepth == 24) warning("ny::X11WC: no 32-bit visuals.");


	depth_iter = xcb_screen_allowed_depths_iterator(screen);
	for(; depth_iter.rem; xcb_depth_next(&depth_iter)) 
	{
		if(depth_iter.data->depth == avDepth)
		{
			//32 > 24 (should not be decided here though)
			//argb > rgba > bgra for 32
			//rgb > bgr for 24
			auto highestScore = 0u;
			auto score = [](ImageDataFormat& f) {
				if(f == ImageDataFormat::argb8888) return 5u;
				else if(f == ImageDataFormat::rgba8888) return 4u;
				else if(f == ImageDataFormat::bgra8888) return 3u;
				else if(f == ImageDataFormat::rgb888) return 2u;
				else if(f == ImageDataFormat::bgr888) return 1u;
				return 0u;
			};

			auto visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
			for(; visual_iter.rem; xcb_visualtype_next(&visual_iter)) 
			{
				//TODO: make requested format dynamic with X11WindowSettings
				auto format = visualToFormat(*visual_iter.data, avDepth);
				if(score(format) > highestScore) visualID_ = visual_iter.data->visual_id;
			}

			break;
		}
	}

	if(!visualID_) throw std::runtime_error("X11WC: failed to find a matching visual.");
	depth_ = avDepth;
}

xcb_connection_t* X11WindowContext::xConnection() const
{
	return appContext().xConnection();
}

x11::EwmhConnection* X11WindowContext::ewmhConnection() const
{
	return appContext().ewmhConnection();
}

void X11WindowContext::refresh()
{
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

void X11WindowContext::hide()
{
    xcb_unmap_window(xConnection(), xWindow_);
}

void X11WindowContext::size(const Vec2ui& size)
{
	xcb_configure_window(xConnection(), xWindow_,
		XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, size.data());
    refresh();
}

void X11WindowContext::position(const Vec2i& position)
{
	std::uint32_t data[2];
	data[0] = position.x;
	data[1] = position.y;

	xcb_configure_window(xConnection(), xWindow(),
		XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, data);
	xcb_flush(xConnection());
}

void X11WindowContext::cursor(const Cursor& curs)
{
	//TODO: xcursor optinal
	//use xcb_render instead to create a cursor (no need to use Xlib)

	//without xcursor:
    // if(curs.type() != CursorType::image && curs.type() != CursorType::none)
    // {
    //     int num = cursorToX11(curs.type());
    //     if(num != -1) cursor(num);
    // }

    if(curs.type() != CursorType::image && curs.type() != CursorType::none)
	{
		auto xdpy = appContext().xDisplay();
		auto name = cursorToXName(curs.type());
		if(!name)
		{
			//TODO: serialize cursor type
			warning("X11WC::cursor: cursor type not supported");
			return;
		}

		if(xCursor_) xcb_free_cursor(xConnection(), xCursor_);

		xCursor_ = XcursorLibraryLoadCursor(xdpy, name);
		xcb_change_window_attributes(xConnection(), xWindow(), XCB_CW_CURSOR, &xCursor_);
	}
	else if(curs.type() == CursorType::image)
	{
		constexpr static auto reqFormat = ImageDataFormat::bgra8888; //TODO: endianess?

		auto xdpy = appContext().xDisplay();
		auto& imgdata = *curs.image();

		auto xcimage = XcursorImageCreate(imgdata.size.x, imgdata.size.y);
		xcimage->xhot = curs.imageHotspot().x;
		xcimage->yhot = curs.imageHotspot().y;

		auto packedStride = imgdata.size.x * imageDataFormatSize(imgdata.format);
		if((imgdata.stride != packedStride && imgdata.stride != 0) || imgdata.format != reqFormat)
		{
			auto pixels = reinterpret_cast<std::uint8_t*>(xcimage->pixels);
			convertFormat(imgdata, reqFormat, *pixels);
		}
		else
		{
			std::memcpy(xcimage->pixels, imgdata.data, imgdata.stride * imgdata.size.y);
		}

		if(xCursor_) xcb_free_cursor(xConnection(), xCursor_);

		xCursor_ = XcursorImageLoadCursor(xdpy, xcimage);
		XcursorImageDestroy(xcimage);
		xcb_change_window_attributes(xConnection(), xWindow(), XCB_CW_CURSOR, &xCursor_);
	}
	else if(curs.type() == CursorType::none)
	{
		auto xconn = xConnection();
		auto cursorPixmap = xcb_generate_id(xconn);
		xcb_create_pixmap(xconn, 1, cursorPixmap, xWindow_, 1, 1);

		if(xCursor_) xcb_free_cursor(xConnection(), xCursor_);
		xCursor_ = xcb_generate_id(xconn);

		xcb_create_cursor(xconn, xCursor_, cursorPixmap, cursorPixmap,
			0, 0, 0, 0, 0, 0, 0, 0);
		xcb_free_pixmap(xconn, cursorPixmap);
		xcb_change_window_attributes(xconn, xWindow_, XCB_CW_CURSOR, &xCursor_);
	}
}

void X11WindowContext::maximize()
{
    addStates(ewmhConnection()->_NET_WM_STATE_MAXIMIZED_VERT,
			ewmhConnection()->_NET_WM_STATE_MAXIMIZED_HORZ);
}

void X11WindowContext::minimize()
{
	// icccm not working on gnome
	// xcb_icccm_wm_hints_t hints;
    // hints.flags = XCB_ICCCM_WM_HINT_STATE;
    // hints.initial_state = XCB_ICCCM_WM_STATE_ICONIC;
    // xcb_icccm_set_wm_hints(xConnection(), xWindow_, &hints);
	
	// not working on gnome
	// addStates(ewmhConnection()->_NET_WM_STATE_HIDDEN);
	
	// xcb_icccm_wm_hints_t hints;
	// xcb_icccm_wm_hints_set_withdrawn(&hints);
    // xcb_icccm_set_wm_hints(xConnection(), xWindow_, &hints);
	
	XIconifyWindow(appContext().xDisplay(), xWindow_, appContext().xDefaultScreenNumber());
	XSync(appContext().xDisplay(), 1);
}

void X11WindowContext::fullscreen()
{
    addStates(ewmhConnection()->_NET_WM_STATE_FULLSCREEN);
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

	//XXX TODO: correct mouse button (index)!
	xcb_ewmh_request_wm_moveresize(ewmhConnection(), 0, xWindow(), xev.root_x, xev.root_y,
		XCB_EWMH_WM_MOVERESIZE_MOVE, XCB_BUTTON_INDEX_1, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL);
}

void X11WindowContext::beginResize(const MouseButtonEvent* ev, WindowEdges edge)
{
	auto* xbev = dynamic_cast<X11EventData*>(ev->data.get());
    if(!xbev) return;
    auto& xev = reinterpret_cast<xcb_button_press_event_t&>(xbev->event);

	xcb_ewmh_moveresize_direction_t x11Edge;
	switch(static_cast<WindowEdge>(edge.value()))
    {
        case WindowEdge::top: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_TOP; break;
        case WindowEdge::left: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_LEFT; break;
        case WindowEdge::bottom: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOM; break;
        case WindowEdge::right: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_RIGHT; break;
        case WindowEdge::topLeft: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_TOPLEFT; break;
        case WindowEdge::topRight: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_TOPRIGHT; break;
        case WindowEdge::bottomLeft: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOMLEFT; break;
        case WindowEdge::bottomRight: x11Edge = XCB_EWMH_WM_MOVERESIZE_SIZE_BOTTOMRIGHT; break;
        default: return;
    }

	//XXX: correct mouse button!
	xcb_ewmh_request_wm_moveresize(ewmhConnection(), 0, xWindow(), xev.root_x, xev.root_y,
		x11Edge, XCB_BUTTON_INDEX_1, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL);
}

void X11WindowContext::icon(const ImageData& img)
{
    if(img.data)
    {
		auto reqFormat = ImageDataFormat::rgba8888;
		auto neededSize = img.size.x * img.size.y;
		auto ownedData = std::make_unique<std::uint32_t[]>(2 + neededSize);

		//the first two ints are width and height
		ownedData[0] = img.size.x;
		ownedData[1] = img.size.y;

		auto size = 2 + neededSize;
		auto imgData = reinterpret_cast<std::uint8_t*>(ownedData.get() + 2);
		convertFormat(img, reqFormat, *imgData);

		auto data = ownedData.get();
		xcb_ewmh_set_wm_icon(ewmhConnection(), XCB_PROP_MODE_REPLACE, xWindow(), size, data);
		xcb_flush(xConnection());
    }
	else
	{
		std::uint32_t buffer[2] = {0};
		xcb_ewmh_set_wm_icon(ewmhConnection(), XCB_PROP_MODE_REPLACE, xWindow(), 2, buffer);
		xcb_flush(xConnection());
	}
}

void X11WindowContext::title(const std::string& str)
{
	xcb_ewmh_set_wm_name(ewmhConnection(), xWindow(), str.size(), str.c_str());
}

NativeHandle X11WindowContext::nativeHandle() const
{
	return NativeHandle(xWindow_);
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

void X11WindowContext::reparentEvent()
{
	position(settings_.position);
}

void X11WindowContext::sizeEvent(nytl::Vec2ui size)
{
	if(drawIntegration_) drawIntegration_->resize(size);
}

bool X11WindowContext::customDecorated() const
{
	return false;
}

WindowCapabilities X11WindowContext::capabilities() const
{
	return WindowCapability::size |
		WindowCapability::fullscreen |
		WindowCapability::minimize |
		WindowCapability::maximize |
		WindowCapability::position |
		WindowCapability::sizeLimits;
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
void X11WindowContext::raise()
{
	const uint32_t values[] = {XCB_STACK_MODE_ABOVE};
    xcb_configure_window(xConnection(), xWindow(), XCB_CONFIG_WINDOW_STACK_MODE, values);
}

void X11WindowContext::lower()
{
	const uint32_t values[] = {XCB_STACK_MODE_BELOW};
    xcb_configure_window(xConnection(), xWindow(), XCB_CONFIG_WINDOW_STACK_MODE, values);
}

void X11WindowContext::requestFocus()
{
	xcb_ewmh_request_change_active_window(ewmhConnection(), 0, xWindow(),
		XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL, XCB_TIME_CURRENT_TIME, XCB_NONE);
}
void X11WindowContext::addStates(xcb_atom_t state1, xcb_atom_t state2)
{
	xcb_ewmh_request_change_wm_state(ewmhConnection(), 0, xWindow(), XCB_EWMH_WM_STATE_ADD,
		state1, state2, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL);
}

void X11WindowContext::removeStates(xcb_atom_t state1, xcb_atom_t state2)
{
	xcb_ewmh_request_change_wm_state(ewmhConnection(), 0, xWindow(), XCB_EWMH_WM_STATE_REMOVE,
		state1, state2, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL);
}

void X11WindowContext::toggleStates(xcb_atom_t state1, xcb_atom_t state2)
{
	xcb_ewmh_request_change_wm_state(ewmhConnection(), 0, xWindow(), XCB_EWMH_WM_STATE_TOGGLE,
		state1, state2, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL);
}

void X11WindowContext::mwmHints(unsigned long deco, unsigned long func, bool d, bool f)
{
    struct x11::MwmHints mhints;
    if(d)
	{
		mwmDecoHints_ = deco;
		mhints.flags |= x11::mwmHintsDeco;
		mhints.decorations = deco;
	}
    if(f)
	{
		mwmFuncHints_ = func;
		mhints.flags |= x11::mwmHintsFunc;
		mhints.functions = func;
	}

	///XXX: use XCB_ATOM_ATOM?
    xcb_change_property(xConnection(), XCB_PROP_MODE_REPLACE, xWindow(),
		appContext().atoms().motifWmHints, XCB_ATOM_CARDINAL, 32, sizeof(x11::MwmHints) / 32,
		reinterpret_cast<std::uint32_t*>(&mhints));
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

	///XXX
	xcb_client_message_event_t ev;
	ev.response_type = XCB_CLIENT_MESSAGE;
	ev.type = ewmhConnection()->_NET_WM_ALLOWED_ACTIONS;

	ev.data.data32[0] = 1; //add
	ev.data.data32[1] = action;
	ev.data.data32[2] = 0;

	xcb_send_event(xConnection(), appContext().xDefaultScreen()->root, );
	*/

	std::uint32_t data[] = {1, action, 0};
	xcb_ewmh_send_client_message(xConnection(), xWindow(), appContext().xDefaultScreen()->root,
		ewmhConnection()->_NET_WM_ALLOWED_ACTIONS, 3, data);
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

	std::uint32_t data[] = {0, action, 0};
	xcb_ewmh_send_client_message(xConnection(), xWindow(), appContext().xDefaultScreen()->root,
		ewmhConnection()->_NET_WM_ALLOWED_ACTIONS, 3, data);
}

std::vector<xcb_atom_t> X11WindowContext::allowedActions() const
{
	auto cookie = xcb_ewmh_get_wm_allowed_actions(ewmhConnection(), xWindow());

	xcb_ewmh_get_atoms_reply_t reply;
	xcb_ewmh_get_wm_allowed_actions_reply(ewmhConnection(), cookie, &reply, nullptr);

    std::vector<xcb_atom_t> ret;
	ret.reserve(reply.atoms_len);

	std::memcpy(ret.data(), reply.atoms, ret.size() * sizeof(std::uint32_t));
	xcb_ewmh_get_atoms_reply_wipe(&reply);

    return ret;
}

void X11WindowContext::refreshStates()
{
    //TODO - needed?
}

void X11WindowContext::transientFor(xcb_window_t other)
{
	xcb_change_property(xConnection(), XCB_PROP_MODE_REPLACE, xWindow(),
        XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 32,	1, &other);
}

void X11WindowContext::xWindowType(xcb_window_t type)
{
	xcb_ewmh_set_wm_window_type(ewmhConnection(), xWindow(), 1, &type);
}

xcb_atom_t X11WindowContext::xWindowType()
{
    return 0;
    //todo
}

void X11WindowContext::overrideRedirect(bool redirect)
{
	std::uint32_t data = redirect;
	xcb_change_window_attributes(xConnection(), xWindow(), XCB_CW_OVERRIDE_REDIRECT, &data);
}

nytl::Vec2ui X11WindowContext::size() const
{
	auto cookie = xcb_get_geometry(xConnection(), xWindow());
	auto geometry = xcb_get_geometry_reply(xConnection(), cookie, nullptr);
	auto ret = nytl::Vec2ui(geometry->width, geometry->height);
	std::free(geometry);
	return ret;
}

xcb_visualtype_t* X11WindowContext::xVisualType() const
{
	if(!visualID_) return nullptr;

	auto depthi = xcb_screen_allowed_depths_iterator(appContext().xDefaultScreen());
	for(; depthi.rem; xcb_depth_next(&depthi)) 
	{
		auto visuali = xcb_depth_visuals_iterator(depthi.data);
		for(; visuali.rem; xcb_visualtype_next(&visuali)) 
		{
			if(visuali.data->visual_id == visualID_) 
				return visuali.data;
		}
	}

	return nullptr;
}

bool X11WindowContext::drawIntegration(X11DrawIntegration* integration)
{
	if(!(bool(drawIntegration_) ^ bool(integration))) return false;
	drawIntegration_ = integration;
	return true;
}

bool X11WindowContext::surface(Surface& surface)
{
	if(drawIntegration_) return false;

	try {
		surface.buffer = std::make_unique<X11BufferSurface>(*this);
		surface.type = SurfaceType::buffer;
		return true;
	} catch(const std::exception& ex) {
		warning("Failed to create x11 surface (BufferSurface) integration: ", ex.what());
		return false;
	}
}

///Draw integration
X11DrawIntegration::X11DrawIntegration(X11WindowContext& wc) : windowContext_(wc)
{
	if(!wc.drawIntegration(this))
		throw std::logic_error("X11DrawIntegration: windowContext already has an integration");
}

X11DrawIntegration::~X11DrawIntegration()
{
	windowContext_.drawIntegration(nullptr);
}

}
