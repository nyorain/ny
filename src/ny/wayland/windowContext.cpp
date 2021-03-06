// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/input.hpp>
#include <ny/wayland/util.hpp>

#include <ny/wayland/protocols/xdg-shell-v5.h>
#include <ny/wayland/protocols/xdg-shell-v6.h>

#include <ny/common/unix.hpp>
#include <ny/mouseContext.hpp>
#include <ny/cursor.hpp>

#include <nytl/vecOps.hpp>
#include <dlg/dlg.hpp>

#include <wayland-cursor.h>

#include <iostream>
#include <cstring>

// TODO: polished implementation of stable xdg protocol
// TODO: more options in window creation for custom roles/extra roles?
//   popup/subsurface? See notes for commits that removed the old impl

namespace ny {

WaylandWindowContext::WaylandWindowContext(WaylandAppContext& ac,
	const WaylandWindowSettings& settings) : appContext_(&ac)
{
	// parse settings
	size_ = settings.size;
	if(size_ == defaultSize) {
		size_ = fallbackSize;
	}

	if(settings.listener) {
		listener(*settings.listener);
	}

	// surface
	wlSurface_ = wl_compositor_create_surface(&ac.wlCompositor());
	if(!wlSurface_) {
		throw std::runtime_error("ny::WaylandWindowContext: could not create wl_surface");
	}
	wl_surface_set_user_data(wlSurface_, this);

	// we prefer xdg shell v6 > v5 > wlShell
	if(ac.xdgShellV6()) {
		createXdgSurfaceV6(settings);
	} else if(ac.xdgShellV5()) {
		createXdgSurfaceV5(settings);
	} else if(ac.wlShell()) {
		createShellSurface(settings);
	} else throw std::runtime_error("ny::WaylandWindowContext: compositor has no shell global");

	switch(settings.initState) {
		case ToplevelState::fullscreen:
			fullscreen();
			break;
		case ToplevelState::maximized:
			maximize();
			break;
		case ToplevelState::minimized:
			minimize();
			break;
		default:
			break;
	}

	cursor(settings.cursor);
}

WaylandWindowContext::~WaylandWindowContext()
{
	appContext().deferred.remove(this);
	if(frameCallback_) {
		wl_callback_destroy(frameCallback_);
	}

	// role
	if(wlShellSurface()) {
		wl_shell_surface_destroy(wlShellSurface_);
	} else if(xdgSurfaceV5()) {
		xdg_surface_destroy(xdgSurfaceV5_);
	} else if(xdgSurfaceV6()) {
		if(xdgToplevelV6()) {
			zxdg_toplevel_v6_destroy(xdgSurfaceV6_.toplevel);
		}

		zxdg_surface_v6_destroy(xdgSurfaceV6_.surface);
	}

	if(wlSurface_) {
		wl_surface_destroy(wlSurface_);
	}
}

void WaylandWindowContext::createShellSurface(const WaylandWindowSettings& ws)
{
	using WWC = WaylandWindowContext;
	constexpr static wl_shell_surface_listener shellSurfaceListener = {
		memberCallback<&WWC::handleShellSurfacePing>,
		memberCallback<&WWC::handleShellSurfaceConfigure>,
		memberCallback<&WWC::handleShellSurfacePopupDone>
	};

	wlShellSurface_ = wl_shell_get_shell_surface(appContext().wlShell(), wlSurface_);
	if(!wlShellSurface_) {
		throw std::runtime_error("ny::WaylandWindowContext: failed to create wl_shell_surface");
	}

	role_ = WaylandSurfaceRole::shell;

	wl_shell_surface_set_user_data(wlShellSurface_, this);
	wl_shell_surface_add_listener(wlShellSurface_, &shellSurfaceListener, this);

	wl_shell_surface_set_title(wlShellSurface_, ws.title.c_str());
	wl_shell_surface_set_class(wlShellSurface_, appContext().appName());
	wl_shell_surface_set_toplevel(wlShellSurface_);

	// draw window for the first time in main loop to make it visible
	appContext().deferred.add([&]{
		DrawEvent de {};
		listener().draw(de);
	}, this);
}

void WaylandWindowContext::createXdgSurfaceV5(const WaylandWindowSettings& ws)
{
	using WWC = WaylandWindowContext;
	constexpr static xdg_surface_listener xdgSurfaceListener = {
		memberCallback<&WWC::handleXdgSurfaceV5Configure>,
		memberCallback<&WWC::handleXdgSurfaceV5Close>
	};

	xdgSurfaceV5_ = xdg_shell_get_xdg_surface(appContext().xdgShellV5(), wlSurface_);
	if(!xdgSurfaceV5_) {
		throw std::runtime_error("ny::WaylandWindowContext: failed to create xdg_surface v5");
	}

	role_ = WaylandSurfaceRole::xdgSurfaceV5;

	xdg_surface_set_user_data(xdgSurfaceV5_, this);
	xdg_surface_add_listener(xdgSurfaceV5_, &xdgSurfaceListener, this);

	xdg_surface_set_title(xdgSurfaceV5_, ws.title.c_str());
	xdg_surface_set_app_id(xdgSurfaceV5_, appContext().appName());

	// TODO: should that be here?
	// draw window for the first time in main loop to make it visible
	appContext().deferred.add([&]{
		DrawEvent de {};
		listener().draw(de);
	}, this);
}

void WaylandWindowContext::createXdgSurfaceV6(const WaylandWindowSettings& ws)
{
	using WWC = WaylandWindowContext;
	constexpr static zxdg_surface_v6_listener xdgSurfaceListener = {
		memberCallback< &WWC::handleXdgSurfaceV6Configure>
	};

	constexpr static zxdg_toplevel_v6_listener xdgToplevelListener = {
		memberCallback<&WWC::handleXdgToplevelV6Configure>,
		memberCallback<&WWC::handleXdgToplevelV6Close>
	};

	// create the xdg surface
	xdgSurfaceV6_.surface = zxdg_shell_v6_get_xdg_surface(appContext().xdgShellV6(), wlSurface_);
	if(!xdgSurfaceV6_.surface)
		throw std::runtime_error("ny::WaylandWindowContext: failed to create xdg_surface v6");

	// create the xdg toplevel for the surface
	xdgSurfaceV6_.toplevel = zxdg_surface_v6_get_toplevel(xdgSurfaceV6_.surface);
	if(!xdgSurfaceV6_.toplevel) {
		throw std::runtime_error("ny::WaylandWindowContext: failed to create xdg_toplevel v6");
	}

	role_ = WaylandSurfaceRole::xdgToplevelV6;
	xdgSurfaceV6_.configured = false;

	zxdg_surface_v6_add_listener(xdgSurfaceV6_.surface, &xdgSurfaceListener, this);
	zxdg_toplevel_v6_add_listener(xdgSurfaceV6_.toplevel, &xdgToplevelListener, this);

	zxdg_toplevel_v6_set_title(xdgSurfaceV6_.toplevel, ws.title.c_str());
	zxdg_toplevel_v6_set_app_id(xdgSurfaceV6_.toplevel, appContext().appName());

	// commit to apply the role
	wl_surface_commit(wlSurface_);
}

void WaylandWindowContext::refresh()
{
	// if there is an active frameCallback just set the flag that we want to refresh
	// as soon as possible
	if(frameCallback_ || (xdgSurfaceV6() && !xdgSurfaceV6_.configured)) {
		refreshFlag_ = true;
		return;
	}

	if(refreshPending_) {
		return;
	}

	// otherwise send a draw event (deferred)
	refreshPending_ = true;
	appContext().deferred.add([&]() {
		// draw window for the first time to make it visible
		refreshPending_ = false;
		DrawEvent de {};
		listener().draw(de);
	}, this);
}

void WaylandWindowContext::show()
{
	dlg_warn("show not supported");
}

void WaylandWindowContext::hide()
{
	dlg_warn("hide not supported");
}

void WaylandWindowContext::size(nytl::Vec2ui size)
{
	size_ = size;
	refresh();
}

void WaylandWindowContext::position(nytl::Vec2i)
{
	dlg_warn("wayland does not support custom positions");
}

void WaylandWindowContext::cursor(const Cursor& cursor)
{
	auto wmc = appContext().waylandMouseContext();
	if(!wmc) {
		dlg_warn("no WaylandMouseContext");
		return;
	}

	if(cursor.type() == Cursor::Type::image) {
		auto img = cursor.image();
		if(!img.data) {
			dlg_warn("invalid image cursor");
			return;
		}

		shmCursorBuffer_ = wayland::ShmBuffer(appContext(), img.size);
		convertFormat(img, waylandToImageFormat(shmCursorBuffer_.format()),
			shmCursorBuffer_.data(), 8u);

		cursorHotspot_ = cursor.imageHotspot();
		cursorSize_ = img.size;
		cursorBuffer_ = &shmCursorBuffer_.wlBuffer();
	} else if(cursor.type() == Cursor::Type::none) {
		cursorHotspot_ = {};
		cursorSize_ = {};
		cursorBuffer_ = {};
	} else {
		auto cursorName = cursorToXName(cursor.type());
		if(!cursorName) {
			auto cname = name(cursor.type());
			dlg_warn("failed to convert cursor type '{}' to xcursor", cname);
			return;
		}

		auto cursorTheme = appContext().wlCursorTheme();
		auto* wlCursor = wl_cursor_theme_get_cursor(cursorTheme, cursorName);
		if(!wlCursor) {
			dlg_warn("failed to retrieve cursor {}", cursorName);
			return;
		}

		// TODO: handle mulitple/animated images
		auto img = wlCursor->images[0];

		cursorBuffer_ = wl_cursor_image_get_buffer(img);

		cursorHotspot_[0] = img->hotspot_y;
		cursorHotspot_[0] = img->hotspot_x;

		cursorSize_[0] = img->width;
		cursorSize_[1] = img->height;
	}

	// update the cursor if needed
	if(wmc->over() == this)
		wmc->cursorBuffer(cursorBuffer_, cursorHotspot_, cursorSize_);
}

void WaylandWindowContext::minSize(nytl::Vec2ui size)
{
	if(xdgToplevelV6()) zxdg_toplevel_v6_set_min_size(xdgSurfaceV6_.toplevel, size[0], size[1]);
	else dlg_warn("this wayland wc has no capability for size limits");
}
void WaylandWindowContext::maxSize(nytl::Vec2ui size)
{
	if(xdgToplevelV6()) zxdg_toplevel_v6_set_max_size(xdgSurfaceV6_.toplevel, size[0], size[1]);
	else dlg_warn("this wayland wc has no capability for size limits");
}

NativeHandle WaylandWindowContext::nativeHandle() const
{
	return {wlSurface_};
}

WindowCapabilities WaylandWindowContext::capabilities() const
{
	// return something else when using xdg shell 6
	// make it dependent on the actual shell used
	auto ret = WindowCapability::size |
		WindowCapability::cursor |
		WindowCapability::customDecoration;

	if(xdgToplevelV6() || xdgSurfaceV5() || wlShellSurface()) {
		ret |= WindowCapability::fullscreen;
		ret |= WindowCapability::title;
		ret |= WindowCapability::beginMove;
		ret |= WindowCapability::beginResize;
		ret |= WindowCapability::maximize;
	}

	if(xdgToplevelV6() || xdgSurfaceV5())
		ret |= WindowCapability::minimize;

	if(xdgToplevelV6())
		ret |= WindowCapability::sizeLimits;

	return ret;
}

void WaylandWindowContext::maximize()
{
	if(wlShellSurface()) wl_shell_surface_set_maximized(wlShellSurface_, nullptr);
	else if(xdgSurfaceV5()) xdg_surface_set_maximized(xdgSurfaceV5_);
	else if(xdgToplevelV6()) zxdg_toplevel_v6_set_maximized(xdgSurfaceV6_.toplevel);
	else dlg_warn("role cannot be maximized");
}

void WaylandWindowContext::fullscreen()
{
	// TODO: which wayland output to choose here?
	constexpr auto method = WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT;

	if(wlShellSurface())
		wl_shell_surface_set_fullscreen(wlShellSurface(), method, 0, nullptr);
	else if(xdgSurfaceV5())
		xdg_surface_set_fullscreen(xdgSurfaceV5_, nullptr);
	else if(xdgToplevelV6())
		zxdg_toplevel_v6_set_fullscreen(xdgSurfaceV6_.toplevel, nullptr);
	else dlg_warn("role does not support fullscreen");
}

void WaylandWindowContext::minimize()
{
	if(xdgSurfaceV5()) xdg_surface_set_minimized(xdgSurfaceV5_);
	else if(xdgToplevelV6()) zxdg_toplevel_v6_set_minimized(xdgSurfaceV6_.toplevel);
	else dlg_warn("role cannot be minimized");
}
void WaylandWindowContext::normalState()
{
	if(wlShellSurface()) {
		wl_shell_surface_set_toplevel(wlShellSurface_);
	} else if(xdgSurfaceV5()) {
		xdg_surface_unset_fullscreen(xdgSurfaceV5_);
		xdg_surface_unset_maximized(xdgSurfaceV5_);
	} else if(xdgToplevelV6()) {
		zxdg_toplevel_v6_unset_fullscreen(xdgSurfaceV6_.toplevel);
		zxdg_toplevel_v6_unset_maximized(xdgSurfaceV6_.toplevel);
	} else {
		dlg_warn("invalid surface role");
	}
}
void WaylandWindowContext::beginMove(const EventData* ev)
{
	if(!appContext().wlSeat()) {
		dlg_warn("beginMove failed: app context has no seat");
		return;
	}

	auto* data = dynamic_cast<const WaylandEventData*>(ev);
	if(!data || !data->serial) {
		dlg_warn("beginMove failed: invalid event data");
		return;
	}

	if(wlShellSurface())
		wl_shell_surface_move(wlShellSurface_, appContext().wlSeat(), data->serial);
	else if(xdgSurfaceV5())
		xdg_surface_move(xdgSurfaceV5_, appContext().wlSeat(), data->serial);
	else if(xdgToplevelV6())
		zxdg_toplevel_v6_move(xdgSurfaceV6_.toplevel, appContext().wlSeat(), data->serial);
	else dlg_warn("role cannot be moved");
}

void WaylandWindowContext::beginResize(const EventData* ev, WindowEdges edge)
{
	if(!appContext().wlSeat()) {
		dlg_warn("beginResize failed: app context has no seat");
		return;
	}

	auto* data = dynamic_cast<const WaylandEventData*>(ev);
	if(!data || !data->serial) {
		dlg_warn("beginResize failed: invalid event data");
		return;
	}

	// wayland core and the xdg protocols have the same edge values
	auto wlEdge = edge.value();
	auto serial = data->serial;

	if(!wlEdge) {
		dlg_warn("beginResize failed: no/invalid edge given");
		return;
	}

	if(wlShellSurface()) {
		wl_shell_surface_resize(wlShellSurface_, appContext().wlSeat(), serial, wlEdge);
	} else if(xdgSurfaceV5()) {
		xdg_surface_resize(xdgSurfaceV5_, appContext().wlSeat(), serial, wlEdge);
	} else if(xdgToplevelV6()) {
		zxdg_toplevel_v6_resize(xdgSurfaceV6_.toplevel, appContext().wlSeat(), serial, wlEdge);
	} else {
		dlg_warn("role cannot be resized");
	}
}

void WaylandWindowContext::title(const char* nt)
{
	if(wlShellSurface()) {
		wl_shell_surface_set_title(wlShellSurface_, nt);
	} else if(xdgSurfaceV5()) {
		xdg_surface_set_title(xdgSurfaceV5_, nt);
	} else if(xdgToplevelV6()) {
		zxdg_toplevel_v6_set_title(xdgSurfaceV6_.toplevel, nt);
	} else {
		dlg_warn("role cannot change title");
	}
}

wl_shell_surface* WaylandWindowContext::wlShellSurface() const
{
	return (role_ == WaylandSurfaceRole::shell) ? wlShellSurface_ : nullptr;
}

xdg_surface* WaylandWindowContext::xdgSurfaceV5() const
{
	return (role_ == WaylandSurfaceRole::xdgSurfaceV5) ? xdgSurfaceV5_ : nullptr;
}

zxdg_surface_v6* WaylandWindowContext::xdgSurfaceV6() const
{
	if(role_ == WaylandSurfaceRole::xdgToplevelV6) {
		return xdgSurfaceV6_.surface;
	}

	return nullptr;
}

zxdg_toplevel_v6* WaylandWindowContext::xdgToplevelV6() const
{
	return (role_ == WaylandSurfaceRole::xdgToplevelV6) ? xdgSurfaceV6_.toplevel : nullptr;
}

wl_display& WaylandWindowContext::wlDisplay() const
{
	return appContext().wlDisplay();
}

void WaylandWindowContext::attachCommit(wl_buffer* buffer)
{
	using WWC = WaylandWindowContext;
	static constexpr wl_callback_listener frameListener {
		memberCallback<&WWC::handleFrameCallback>
	};

	frameCallback_ = wl_surface_frame(wlSurface_);
	wl_callback_add_listener(frameCallback_, &frameListener, this);
	wl_surface_damage(wlSurface_, 0, 0, size_[0], size_[1]);
	wl_surface_attach(wlSurface_, buffer, 0, 0);

	wl_surface_commit(wlSurface_);
}

Surface WaylandWindowContext::surface()
{
	return {};
}

void WaylandWindowContext::reparseState(const wl_array& states)
{
	auto newXdgState = ToplevelState::normal;
	for(auto i = 0u; i < states.size / sizeof(uint32_t); ++i) {
		auto wlState = (static_cast<uint32_t*>(states.data))[i];
		auto toplevelState = waylandToState(wlState);
		if(toplevelState == ToplevelState::fullscreen) {
			newXdgState = toplevelState;
			break;
		} else if(toplevelState == ToplevelState::maximized) {
			// don't break here, wait for fullscreen
			// if that is set as well; prefer fullscreen
			currentXdgState_ = toplevelState;
		}
	}

	if(newXdgState != currentXdgState_) {
		currentXdgState_ = newXdgState;

		StateEvent se;
		se.state = currentXdgState_;
		se.shown = true; // can this ever be false?
		listener().state(se);
	}
}

void WaylandWindowContext::handleFrameCallback(wl_callback*, uint32_t)
{
	if(frameCallback_) {
		wl_callback_destroy(frameCallback_);
		frameCallback_ = nullptr;
	}

	if(refreshFlag_) {
		refreshFlag_ = false;

		DrawEvent de {};
		listener().draw(de);
	}
}

void WaylandWindowContext::handleShellSurfacePing(wl_shell_surface*, uint32_t serial)
{
	wl_shell_surface_pong(wlShellSurface(), serial);
}

void WaylandWindowContext::handleShellSurfaceConfigure(wl_shell_surface*, uint32_t edges,
	int32_t width, int32_t height)
{
	// TODO: can width/height be 0?

	nytl::unused(edges); // TODO?

	size_[0] = width;
	size_[1] = height;

	SizeEvent se;
	se.size = size_;
	listener().resize(se);

	size(size_);
}

void WaylandWindowContext::handleShellSurfacePopupDone(wl_shell_surface*)
{
	// TODO
	CloseEvent ce;
	listener().close(ce);
}

void WaylandWindowContext::handleXdgSurfaceV5Configure(xdg_surface*, int32_t width, int32_t height,
	wl_array* states, uint32_t serial)
{
	// TODO: somehow check if size has really changed.
	// if not, dont send size event and redraw
	reparseState(*states);
	if(width && height) {
		size_[0] = width;
		size_[1] = height;

		SizeEvent se;
		se.size = size_;
		listener().resize(se);
	}

	xdg_surface_ack_configure(xdgSurfaceV5(), serial);
	refresh();
}

void WaylandWindowContext::handleXdgSurfaceV5Close(xdg_surface*)
{
	CloseEvent ce;
	listener().close(ce);
}

// TODO: defer the whole configure event handling. Only handle the last
// event received (should speed up resizing)
void WaylandWindowContext::handleXdgSurfaceV6Configure(zxdg_surface_v6*, uint32_t serial)
{
	// TODO: somehow check if size has really changed.
	// only then send redraw/resize events

	xdgSurfaceV6_.configured = true;
	zxdg_surface_v6_ack_configure(xdgSurfaceV6(), serial);

	SizeEvent se;
	se.size = size_;
	listener().resize(se);

	refresh();
}

void WaylandWindowContext::handleXdgToplevelV6Configure(zxdg_toplevel_v6*, int32_t width,
	int32_t height, wl_array* states)
{
	reparseState(*states);
	if(!width || !height) {
		// in this case we simply keep the (copied from WindowSettings)
		// width and height
		return;
	}

	// we store the new size but not yet actually resize/redraw the window
	// this should/can only be done after we recevie the surfaceV6Configure event
	// we will also then send the size event
	size_[0] = width;
	size_[1] = height;
}

void WaylandWindowContext::handleXdgToplevelV6Close(zxdg_toplevel_v6*)
{
	CloseEvent ce;
	listener().close(ce);
}

} // namespace ny
