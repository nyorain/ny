// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/windowContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/x11/defs.hpp>
#include <ny/x11/appContext.hpp>

#include <ny/common/unix.hpp>
#include <ny/log.hpp>
#include <ny/cursor.hpp>
#include <ny/mouseContext.hpp>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_image.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xlib.h>

#include <cstring> // std::memcpy

namespace ny {

//windowContext
X11WindowContext::X11WindowContext(X11AppContext& ctx, const X11WindowSettings& settings)
{
	create(ctx, settings);
}

X11WindowContext::~X11WindowContext()
{
	if(xWindow_) {
		appContext().unregisterContext(xWindow_);
		xcb_destroy_window(&xConnection(), xWindow_);
	}

	if(xColormap_) xcb_free_colormap(&xConnection(), xCursor_);
	if(xCursor_) xcb_free_cursor(&xConnection(), xCursor_);
	xcb_flush(&xConnection());
}

void X11WindowContext::create(X11AppContext& ctx, const X11WindowSettings& settings)
{
	appContext_ = &ctx;
	settings_ = settings;
	auto& xconn = xConnection();

	if(settings.listener) listener(*settings.listener);

	// TODO: query visual id for native handle
	if(settings.nativeHandle) xWindow_ = settings.nativeHandle;
	else createWindow(settings);

	appContext_->registerContext(xWindow_, *this);

	if(!settings.parent) {
		auto protocols = ewmhConnection().WM_PROTOCOLS;
		auto supportedProtocols = appContext_->atoms().wmDeleteWindow;
		if(!settings.title.empty()) title(settings.title);
		xcb_change_property(&xconn, XCB_PROP_MODE_REPLACE, xWindow_, protocols,
				XCB_ATOM_ATOM, 32, 1, &supportedProtocols);
		xcb_change_property(&xconn, XCB_PROP_MODE_REPLACE, xWindow_, XCB_ATOM_WM_NAME,
				XCB_ATOM_STRING, 8, settings.title.size(), settings.title.c_str());
	}

	// signal that this window understands the xdnd protocol
	// version 5 of the xdnd protocol is supported
	if(settings.droppable) {
		static constexpr auto version = 5u;
		xcb_change_property(&xconn, XCB_PROP_MODE_REPLACE, xWindow_, appContext().atoms().xdndAware,
			XCB_ATOM_ATOM, 32, 1, reinterpret_cast<const unsigned char*>(&version));
	}

	// apply init settings
	cursor(settings.cursor);
	if(settings.show) show();

	if(settings.initState == ToplevelState::maximized) maximize();
	else if(settings.initState == ToplevelState::minimized) minimize();
	else if(settings.initState == ToplevelState::fullscreen) fullscreen();

	// make sure windows is mapped and set to correct state
	xcb_flush(&xconn);
}

void X11WindowContext::createWindow(const X11WindowSettings& settings)
{
	if(!visualID_) initVisual(settings);
	auto visualtype = xVisualType();
	if(!visualtype)
		throw std::runtime_error("ny::X11WindowContext: failed to retrieve the visualtype");

	auto vid = visualtype->visual_id;
	auto& xconn = appContext_->xConnection();
	auto& xscreen = appContext_->xDefaultScreen();

	auto pos = settings.position;
	if(pos == defaultPosition) pos = fallbackPosition;

	auto size = settings.size;
	if(size == defaultSize) size = fallbackSize;

	xcb_window_t xparent = settings.parent.uint64();
	if(!xparent) xparent = xscreen.root;

	auto colormap = xcb_generate_id(&xconn);
	auto cookie = xcb_create_colormap_checked(&xconn, XCB_COLORMAP_ALLOC_NONE, colormap,
		xscreen.root, vid);
	errorCategory().checkThrow(cookie, "ny::X11WindowContext: create_colormap failed");

	xColormap_ = colormap;

	std::uint32_t eventmask =
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS |
		XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION |
		XCB_EVENT_MASK_FOCUS_CHANGE;

	// Setting the background pixel here may introduce flicker but may fix issues
	// with creating opengl windows.
	std::uint32_t valuelist[] = {0, eventmask, colormap, 0};
	std::uint32_t valuemask = XCB_CW_COLORMAP | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK;

	auto window = xcb_generate_id(&xconn);
	cookie = xcb_create_window_checked(&xconn, depth_, window, xparent, pos[0], pos[1],
		size[0], size[1], 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, vid, valuemask, valuelist);
	errorCategory().checkThrow(cookie, "ny::X11WindowContext: create_window failed");
	xWindow_ = window;
}

void X11WindowContext::initVisual(const X11WindowSettings& settings)
{
	dlg_source("xwc"_module, "initVisual"_scope);
	static constexpr auto novis = "ny::X11WindowContext::initVisual: ny no 24 or 32 bit visuals";
	static constexpr auto nofound = "ny::X11WindowContext::initVisual: no matching visuals";

	visualID_ = 0u;
	auto& screen = appContext().xDefaultScreen();
	auto avDepth = 0u;

	auto depth_iter = xcb_screen_allowed_depths_iterator(&screen);
	for(; depth_iter.rem; xcb_depth_next(&depth_iter)) {
		if(depth_iter.data->depth == 32) {
			avDepth = 32;
			if(settings.transparent) break;
		}

		if(!avDepth && depth_iter.data->depth == 24) {
			avDepth = 24;
			if(!settings.transparent) break;
		}
	}

	if(avDepth == 0u)
		throw std::runtime_error(novis);
	else if(settings.transparent && avDepth == 24)
		ny_warn("transparent window but no 32 bit visual");
	else if(!settings.transparent && avDepth == 32)
		ny_info("not-transparent window, but only 32 bits visuals");

	// argb > rgba > bgra for 32
	// rgb > bgr for 24
	depth_iter = xcb_screen_allowed_depths_iterator(&screen);
	for(; depth_iter.rem; xcb_depth_next(&depth_iter)) {
		if(depth_iter.data->depth != avDepth) continue;

		auto highestScore = 0u;
		auto score = [](const ImageFormat& f) {
			if(f == ImageFormat::argb8888) return 3u;
			else if(f == ImageFormat::rgba8888) return 2u;
			else if(f == ImageFormat::bgra8888) return 1u;

			else if(f == ImageFormat::rgb888) return 2u;
			else if(f == ImageFormat::bgr888) return 1u;
			return 1u;
		};

		// TODO: make requested format dynamic with X11WindowSettings
		// i.e. make it possible to request a certain format
		auto visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
		for(; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
			auto format = x11::visualToFormat(*visual_iter.data, avDepth);
			if(score(format) > highestScore) visualID_ = visual_iter.data->visual_id;
		}

		break;
	}

	if(!visualID_) throw std::runtime_error(nofound);
	depth_ = avDepth;
}

xcb_connection_t& X11WindowContext::xConnection() const
{
	return appContext().xConnection();
}

x11::EwmhConnection& X11WindowContext::ewmhConnection() const
{
	return appContext().ewmhConnection();
}

const X11ErrorCategory& X11WindowContext::errorCategory() const
{
	return appContext().errorCategory();
}

void X11WindowContext::refresh()
{
	xcb_expose_event_t ev{};

	ev.response_type = XCB_EXPOSE;
	ev.window = xWindow();

	xcb_send_event(&xConnection(), 0, xWindow(), XCB_EVENT_MASK_EXPOSURE, (const char*)&ev);
	xcb_flush(&xConnection());
}

void X11WindowContext::show()
{
	xcb_map_window(&xConnection(), xWindow_);
	refresh();
}

void X11WindowContext::hide()
{
	xcb_unmap_window(&xConnection(), xWindow_);
}

void X11WindowContext::size(nytl::Vec2ui size)
{
	xcb_configure_window(&xConnection(), xWindow_,
		XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, size.data());
	refresh();
}

void X11WindowContext::position(nytl::Vec2i position)
{
	std::uint32_t data[2];
	data[0] = position[0];
	data[1] = position[1];

	xcb_configure_window(&xConnection(), xWindow(),
		XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, data);
	xcb_flush(&xConnection());
}

void X11WindowContext::cursor(const Cursor& curs)
{
	// TODO: make xcursor optinal
	// use xcb_render instead to create a cursor (no need to use Xlib)?

	auto& xdpy = appContext().xDisplay();
	if(curs.type() != CursorType::image && curs.type() != CursorType::none) {
		auto xname = cursorToXName(curs.type());
		if(!xname) {
			auto cname = name(curs.type());
			ny_warn("::xwc::cursor"_src "{} not supported", cname);
			return;
		}

		if(xCursor_) xcb_free_cursor(&xConnection(), xCursor_);
		xCursor_ = XcursorLibraryLoadCursor(&xdpy, xname);
		xcb_change_window_attributes(&xConnection(), xWindow(), XCB_CW_CURSOR, &xCursor_);
	} else if(curs.type() == CursorType::image) {
		auto img = curs.image();

		auto xcimage = XcursorImageCreate(img.size[0], img.size[1]);
		xcimage->xhot = curs.imageHotspot()[0];
		xcimage->yhot = curs.imageHotspot()[1];

		auto pixels = reinterpret_cast<std::uint8_t*>(xcimage->pixels);
		convertFormat(img, ImageFormat::argb8888, *pixels, 8u);
		if(xCursor_) xcb_free_cursor(&xConnection(), xCursor_);

		xCursor_ = XcursorImageLoadCursor(&xdpy, xcimage);
		XcursorImageDestroy(xcimage);
		xcb_change_window_attributes(&xConnection(), xWindow(), XCB_CW_CURSOR, &xCursor_);
	} else if(curs.type() == CursorType::none) {
		auto& xconn = xConnection();
		auto cursorPixmap = xcb_generate_id(&xconn);
		xcb_create_pixmap(&xconn, 1, cursorPixmap, xWindow_, 1, 1);

		if(xCursor_) xcb_free_cursor(&xConnection(), xCursor_);
		xCursor_ = xcb_generate_id(&xconn);

		xcb_create_cursor(&xconn, xCursor_, cursorPixmap, cursorPixmap,
			0, 0, 0, 0, 0, 0, 0, 0);
		xcb_free_pixmap(&xconn, cursorPixmap);
		xcb_change_window_attributes(&xconn, xWindow_, XCB_CW_CURSOR, &xCursor_);
	}
}

void X11WindowContext::maximize()
{
	addStates(ewmhConnection()._NET_WM_STATE_MAXIMIZED_VERT,
			ewmhConnection()._NET_WM_STATE_MAXIMIZED_HORZ);
}

void X11WindowContext::minimize()
{
	// TODO: use xcb/some modern api here (?)

	// icccm not working on gnome
	// xcb_icccm_wm_hints_t hints;
	// hints.flags = XCB_ICCCM_WM_HINT_STATE;
	// hints.initial_state = XCB_ICCCM_WM_STATE_ICONIC;
	// xcb_icccm_set_wm_hints(&xConnection(), xWindow_, &hints);

	// not working on gnome
	// addStates(ewmhConnection()->_NET_WM_STATE_HIDDEN);

	// xcb_icccm_wm_hints_t hints;
	// xcb_icccm_wm_hints_set_withdrawn(&hints);
	// xcb_icccm_set_wm_hints(&xConnection(), xWindow_, &hints);

	::XIconifyWindow(&appContext().xDisplay(), xWindow_, appContext().xDefaultScreenNumber());
	::XSync(&appContext().xDisplay(), 1);
}

void X11WindowContext::fullscreen()
{
	addStates(ewmhConnection()._NET_WM_STATE_FULLSCREEN);
}

void X11WindowContext::normalState()
{
	// set nomrla wm hints state
	xcb_icccm_wm_hints_t hints;
	hints.flags = XCB_ICCCM_WM_HINT_STATE;
	hints.initial_state = XCB_ICCCM_WM_STATE_NORMAL;
	xcb_icccm_set_wm_hints(&xConnection(), xWindow_, &hints);

	// remove fullscreen, maximized state
	removeStates(ewmhConnection()._NET_WM_STATE_FULLSCREEN);
	removeStates(ewmhConnection()._NET_WM_STATE_MAXIMIZED_VERT,
		ewmhConnection()._NET_WM_STATE_MAXIMIZED_HORZ);
}

void X11WindowContext::beginMove(const EventData* ev)
{
	// TODO: also try to do it without given event data?
	// we could figure the current mouse position out

	auto* xbev = dynamic_cast<const X11EventData*>(ev);
	if(!xbev) {
		ny_warn("::xwc::beginMove"_src, "no event data given");
		return;
	}

	auto& xev = reinterpret_cast<const xcb_button_press_event_t&>(xbev->event);

	XEvent event;
	memset(&event, 0, sizeof(event));
	event.xclient.type = ClientMessage;
	event.xclient.display = &appContext().xDisplay();
	event.xclient.window = xWindow();
	event.xclient.message_type = ewmhConnection()._NET_WM_MOVERESIZE;
	event.xclient.format = 32;
	event.xclient.data.l[0] = xev.root_x;
	event.xclient.data.l[1] = xev.root_y;
	event.xclient.data.l[2] = XCB_EWMH_WM_MOVERESIZE_MOVE;
	event.xclient.data.l[3] = xev.detail;
	event.xclient.data.l[4] = 0;

	auto root = DefaultRootWindow(&appContext().xDisplay());
	XSendEvent(&appContext().xDisplay(), root, False,
	     SubstructureRedirectMask | SubstructureNotifyMask,
	     &event);
	XFlush(&appContext().xDisplay());
}

void X11WindowContext::beginResize(const EventData* ev, WindowEdges edge)
{
	// TODO: also try to do it without given event data?
	// we could figure the current mouse position out

	auto* xbev = dynamic_cast<const X11EventData*>(ev);
	if(!xbev) return;

	auto& xev = reinterpret_cast<const xcb_button_press_event_t&>(xbev->event);

	xcb_ewmh_moveresize_direction_t x11Edge;
	switch(static_cast<WindowEdge>(edge.value())) {
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

	auto index = static_cast<xcb_button_index_t>(xev.detail);
	xcb_ewmh_request_wm_moveresize(&ewmhConnection(), 0, xWindow(), xev.root_x, xev.root_y,
		x11Edge, index, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL);
	xcb_flush(&xConnection());
}

void X11WindowContext::icon(const Image& img)
{
	if(img.data) {
		auto reqFormat = ImageFormat::rgba8888;
		auto neededSize = img.size[0] * img.size[1];
		auto ownedData = std::make_unique<std::uint32_t[]>(2 + neededSize);

		//the first two ints are width and height
		ownedData[0] = img.size[0];
		ownedData[1] = img.size[1];

		auto size = 2 + neededSize;
		auto imgData = reinterpret_cast<std::uint8_t*>(ownedData.get() + 2);
		convertFormat(img, reqFormat, *imgData);

		auto data = ownedData.get();
		xcb_ewmh_set_wm_icon(&ewmhConnection(), XCB_PROP_MODE_REPLACE, xWindow(), size, data);
		xcb_flush(&xConnection());
	} else {
		std::uint32_t buffer[2] = {0};
		xcb_ewmh_set_wm_icon(&ewmhConnection(), XCB_PROP_MODE_REPLACE, xWindow(), 2, buffer);
		xcb_flush(&xConnection());
	}
}

void X11WindowContext::title(std::string_view title)
{
	xcb_ewmh_set_wm_name(&ewmhConnection(), xWindow(), title.size(), title.data());
}

NativeHandle X11WindowContext::nativeHandle() const
{
	return NativeHandle(xWindow_);
}

void X11WindowContext::minSize(nytl::Vec2ui size)
{
	xcb_size_hints_t hints {};
	hints.min_width = size[0];
	hints.min_height = size[1];
	hints.flags = XCB_ICCCM_SIZE_HINT_P_MIN_SIZE;
	xcb_icccm_set_wm_normal_hints(&xConnection(), xWindow(), &hints);
}

void X11WindowContext::maxSize(nytl::Vec2ui size)
{
	xcb_size_hints_t hints {};
	hints.max_width = size[0];
	hints.max_height = size[1];
	hints.flags = XCB_ICCCM_SIZE_HINT_P_MAX_SIZE;
	xcb_icccm_set_wm_normal_hints(&xConnection(), xWindow(), &hints);
}

void X11WindowContext::reparentEvent()
{
	position(settings_.position);
}

void X11WindowContext::customDecorated(bool set)
{
	typedef struct {
		unsigned long flags;
		unsigned long functions;
		unsigned long decorations;
		long input_mode;
		unsigned long status;
	} MotifWmHints;

	MotifWmHints motif_hints {};
	motif_hints.flags = 2u;
	motif_hints.decorations = set ? 0u : 1u;

	customDecorated_ = set;

	auto hint_atom = appContext().atoms().motifWmHints;
	XChangeProperty(&appContext().xDisplay(), xWindow(), hint_atom, hint_atom, 32, PropModeReplace,
		reinterpret_cast<unsigned char*>(&motif_hints), sizeof(MotifWmHints)/sizeof(long));
}

bool X11WindowContext::customDecorated() const
{
	return customDecorated_;
}

WindowCapabilities X11WindowContext::capabilities() const
{
	// TODO; query if curstom and server decoration are really supported!
	return WindowCapability::size |
		WindowCapability::fullscreen |
		WindowCapability::minimize |
		WindowCapability::maximize |
		WindowCapability::position |
		WindowCapability::sizeLimits |
		WindowCapability::icon |
		WindowCapability::cursor |
		WindowCapability::title |
		WindowCapability::beginMove |
		WindowCapability::beginResize |
		WindowCapability::visibility |
		WindowCapability::customDecoration |
		WindowCapability::serverDecoration;
}

// x11 specific
void X11WindowContext::raise()
{
	const uint32_t values[] = {XCB_STACK_MODE_ABOVE};
	xcb_configure_window(&xConnection(), xWindow(), XCB_CONFIG_WINDOW_STACK_MODE, values);
}

void X11WindowContext::lower()
{
	const uint32_t values[] = {XCB_STACK_MODE_BELOW};
	xcb_configure_window(&xConnection(), xWindow(), XCB_CONFIG_WINDOW_STACK_MODE, values);
}

void X11WindowContext::requestFocus()
{
	xcb_ewmh_request_change_active_window(&ewmhConnection(), 0, xWindow(),
		XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL, XCB_TIME_CURRENT_TIME, XCB_NONE);
}
void X11WindowContext::addStates(xcb_atom_t state1, xcb_atom_t state2)
{
	xcb_ewmh_request_change_wm_state(&ewmhConnection(), 0, xWindow(), XCB_EWMH_WM_STATE_ADD,
		state1, state2, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL);
}

void X11WindowContext::removeStates(xcb_atom_t state1, xcb_atom_t state2)
{
	xcb_ewmh_request_change_wm_state(&ewmhConnection(), 0, xWindow(), XCB_EWMH_WM_STATE_REMOVE,
		state1, state2, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL);
}

void X11WindowContext::toggleStates(xcb_atom_t state1, xcb_atom_t state2)
{
	xcb_ewmh_request_change_wm_state(&ewmhConnection(), 0, xWindow(), XCB_EWMH_WM_STATE_TOGGLE,
		state1, state2, XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL);
}

void X11WindowContext::addAllowedAction(xcb_atom_t action)
{
	std::uint32_t data[] = {1, action, 0};
	xcb_ewmh_send_client_message(&xConnection(), xWindow(), appContext().xDefaultScreen().root,
		ewmhConnection()._NET_WM_ALLOWED_ACTIONS, 3, data);
}

void X11WindowContext::removeAllowedAction(xcb_atom_t action)
{
	std::uint32_t data[] = {0, action, 0};
	xcb_ewmh_send_client_message(&xConnection(), xWindow(), appContext().xDefaultScreen().root,
		ewmhConnection()._NET_WM_ALLOWED_ACTIONS, 3, data);
}

std::vector<xcb_atom_t> X11WindowContext::allowedActions() const
{
	auto cookie = xcb_ewmh_get_wm_allowed_actions(&ewmhConnection(), xWindow());

	xcb_ewmh_get_atoms_reply_t reply;
	xcb_ewmh_get_wm_allowed_actions_reply(&ewmhConnection(), cookie, &reply, nullptr);

	std::vector<xcb_atom_t> ret;
	ret.reserve(reply.atoms_len);

	std::memcpy(ret.data(), reply.atoms, ret.size() * sizeof(std::uint32_t));
	xcb_ewmh_get_atoms_reply_wipe(&reply);

	return ret;
}

void X11WindowContext::transientFor(xcb_window_t other)
{
	xcb_change_property(&xConnection(), XCB_PROP_MODE_REPLACE, xWindow(),
		XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 32,	1, &other);
}

void X11WindowContext::xWindowType(xcb_window_t type)
{
	xcb_ewmh_set_wm_window_type(&ewmhConnection(), xWindow(), 1, &type);
}

void X11WindowContext::overrideRedirect(bool redirect)
{
	std::uint32_t data = redirect;
	xcb_change_window_attributes(&xConnection(), xWindow(), XCB_CW_OVERRIDE_REDIRECT, &data);
}

nytl::Vec2ui X11WindowContext::size() const
{
	auto cookie = xcb_get_geometry(&xConnection(), xWindow());
	auto geometry = xcb_get_geometry_reply(&xConnection(), cookie, nullptr);
	auto ret = nytl::Vec2ui{geometry->width, geometry->height};
	std::free(geometry);
	return ret;
}

xcb_visualtype_t* X11WindowContext::xVisualType() const
{
	if(!visualID_) return nullptr;

	auto depthi = xcb_screen_allowed_depths_iterator(&appContext().xDefaultScreen());
	for(; depthi.rem; xcb_depth_next(&depthi)) {
		auto visuali = xcb_depth_visuals_iterator(depthi.data);
		for(; visuali.rem; xcb_visualtype_next(&visuali)) {
			if(visuali.data->visual_id == visualID_)
				return visuali.data;
		}
	}

	return nullptr;
}

Surface X11WindowContext::surface()
{
	return {};
}

} // namespace ny
