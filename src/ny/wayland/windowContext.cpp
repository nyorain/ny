// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/input.hpp>
#include <ny/wayland/util.hpp>

#include <ny/wayland/protocols/xdg-decoration-unstable-v1.h>
#include <ny/wayland/protocols/xdg-shell-unstable-v6.h>
#include <ny/wayland/protocols/xdg-shell.h>

#include <ny/common/unix.hpp>
#include <ny/mouseContext.hpp>
#include <ny/cursor.hpp>

#include <nytl/vecOps.hpp>
#include <dlg/dlg.hpp>

#include <wayland-cursor.h>

#include <iostream>
#include <cstring>

namespace ny {

WaylandWindowContext::WaylandWindowContext(WaylandAppContext& ac,
		const WaylandWindowSettings& settings) : appContext_(&ac) {

	// parse settings
	size_ = settings.size;
	if(size_ == defaultSize) {
		size_ = fallbackSize;

		// send initial size since that isn't known yet to app
		appContext().deferred.add([&]{
			SizeEvent se {};
			se.size = size_;
			listener().resize(se);
		}, this);
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

	if(ac.xdgWmBase()) {
		createXdgToplevel(settings);
	} else if(ac.xdgShellV6()) {
		createXdgToplevelV6(settings);
	} else if(ac.wlShell()) {
		createShellSurface(settings);
	} else {
		throw std::runtime_error("ny::WaylandWindowContext: compositor has no shell global");
	}

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

WaylandWindowContext::~WaylandWindowContext() {
	appContext().destroyed(*this);
	if(frameCallback_) {
		wl_callback_destroy(frameCallback_);
	}

	// role
	if(wlShellSurface()) {
		wl_shell_surface_destroy(wlShellSurface_);
	} else if(xdgSurfaceV6()) {
		if(xdgToplevelV6()) {
			zxdg_toplevel_v6_destroy(xdgToplevelV6_.toplevel);
		}

		zxdg_surface_v6_destroy(xdgToplevelV6_.surface);
	} else if(xdgSurface()) {
		if(xdgDecoration()) {
			zxdg_toplevel_decoration_v1_destroy(xdgDecoration());
		}
		if(xdgToplevel()) {
			xdg_toplevel_destroy(xdgToplevel());
		}
		xdg_surface_destroy(xdgSurface());
	}

	if(wlSurface_) {
		wl_surface_destroy(wlSurface_);
	}
}

void WaylandWindowContext::createShellSurface(const WaylandWindowSettings& ws) {
	using WWC = WaylandWindowContext;
	constexpr static wl_shell_surface_listener shellSurfaceListener = {
		memberCallback<&WWC::handleShellSurfacePing>,
		memberCallback<&WWC::handleShellSurfaceConfigure>,
		memberCallback<&WWC::handleShellSurfacePopupDone>
	};

	wlShellSurface_ = wl_shell_get_shell_surface(appContext().wlShell(), wlSurface_);
	if(!wlShellSurface_) {
		throw std::runtime_error("ny::WaylandWindowContext: "
			"failed to create wl_shell_surface");
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

	if(ws.customDecorated && !*ws.customDecorated) {
		dlg_warn("can't use server side decorations (using wl_shell)");
	}
}

void WaylandWindowContext::createXdgToplevel(const WaylandWindowSettings& ws) {
	using WWC = WaylandWindowContext;
	constexpr static xdg_surface_listener xdgSurfaceListener = {
		memberCallback<&WWC::handleXdgSurfaceConfigure>
	};

	constexpr static xdg_toplevel_listener xdgToplevelListener = {
		memberCallback<&WWC::handleXdgToplevelConfigure>,
		memberCallback<&WWC::handleXdgToplevelClose>
	};

	// create the xdg surface
	role_ = WaylandSurfaceRole::xdgToplevel;
	xdgToplevel_ = {};
	xdgToplevel_.configured = false;
	xdgToplevel_.surface = xdg_wm_base_get_xdg_surface(appContext().xdgWmBase(),
		wlSurface_);
	if(!xdgSurface()) {
		throw std::runtime_error("ny::WaylandWindowContext: "
			"failed to create xdg_surface");
	}

	// create the xdg toplevel for the surface
	xdgToplevel_.toplevel = xdg_surface_get_toplevel(xdgSurface());
	if(!xdgToplevel()) {
		throw std::runtime_error("ny::WaylandWindowContext: "
			"failed to create xdg_toplevel");
	}

	xdg_surface_add_listener(xdgSurface(), &xdgSurfaceListener, this);
	xdg_toplevel_add_listener(xdgToplevel(), &xdgToplevelListener, this);

	xdg_toplevel_set_title(xdgToplevel(), ws.title.c_str());
	xdg_toplevel_set_app_id(xdgToplevel(), appContext().appName());

	// commit to apply the role
	wl_surface_commit(wlSurface_);

	// custom decoration
	// NOTE: when customDecoration is desired we don't explicitly set it
	// via the protocol. Any advantage in doing so?
	if(ws.customDecorated && !*ws.customDecorated) {
		if(!appContext().xdgDecorationManager()) {
			dlg_warn("compositor doesn't support xdg-decoration, "
				"having to use client side decorations");
		} else {
			using WWC = WaylandWindowContext;
			constexpr static zxdg_toplevel_decoration_v1_listener xdgDecoListener = {
				memberCallback<&WWC::handleXdgDecorationConfigure>,
			};

			xdgToplevel_.decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
				appContext().xdgDecorationManager(), xdgToplevel());
			zxdg_toplevel_decoration_v1_add_listener(xdgDecoration(),
				&xdgDecoListener, this);
			zxdg_toplevel_decoration_v1_set_mode(xdgDecoration(),
				ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
		}
	}
}

void WaylandWindowContext::createXdgToplevelV6(const WaylandWindowSettings& ws) {
	using WWC = WaylandWindowContext;
	constexpr static zxdg_surface_v6_listener xdgSurfaceListener = {
		memberCallback<&WWC::handleXdgSurfaceV6Configure>
	};

	constexpr static zxdg_toplevel_v6_listener xdgToplevelListener = {
		memberCallback<&WWC::handleXdgToplevelV6Configure>,
		memberCallback<&WWC::handleXdgToplevelV6Close>
	};

	// create the xdg surface
	role_ = WaylandSurfaceRole::xdgToplevelV6;
	xdgToplevelV6_ = {};
	xdgToplevelV6_.configured = false;
	xdgToplevelV6_.surface = zxdg_shell_v6_get_xdg_surface(
		appContext().xdgShellV6(), wlSurface_);
	if(!xdgSurfaceV6()) {
		throw std::runtime_error("ny::WaylandWindowContext: "
			"failed to create xdg_surface v6");
	}

	// create the xdg toplevel for the surface
	xdgToplevelV6_.toplevel = zxdg_surface_v6_get_toplevel(xdgSurfaceV6());
	if(!xdgToplevelV6()) {
		throw std::runtime_error("ny::WaylandWindowContext: "
			"failed to create xdg_toplevel v6");
	}

	zxdg_surface_v6_add_listener(xdgSurfaceV6(), &xdgSurfaceListener, this);
	zxdg_toplevel_v6_add_listener(xdgToplevelV6(), &xdgToplevelListener, this);

	zxdg_toplevel_v6_set_title(xdgToplevelV6(), ws.title.c_str());
	zxdg_toplevel_v6_set_app_id(xdgToplevelV6(), appContext().appName());

	// commit to apply the role
	wl_surface_commit(wlSurface_);

	if(ws.customDecorated && !*ws.customDecorated) {
		dlg_warn("can't use server side decorations (using xdg shell v6)");
	}
}

void WaylandWindowContext::refresh() {
	// if there is an active frameCallback just set the flag that we want to refresh
	// as soon as possible
	if(frameCallback_ || (xdgSurfaceV6() && !xdgToplevelV6_.configured)) {
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

void WaylandWindowContext::show() {
	dlg_warn("show not supported");
}

void WaylandWindowContext::hide() {
	dlg_warn("hide not supported");
}

void WaylandWindowContext::size(nytl::Vec2ui size) {
	size_ = size;
	refresh();
}

void WaylandWindowContext::position(nytl::Vec2i) {
	dlg_warn("wayland does not support custom positions");
}

void WaylandWindowContext::cursor(const Cursor& cursor) {
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
	if(wmc->over() == this) {
		wmc->cursorBuffer(cursorBuffer_, cursorHotspot_, cursorSize_);
	}
}

void WaylandWindowContext::minSize(nytl::Vec2ui size) {
	if(xdgToplevelV6()) {
		zxdg_toplevel_v6_set_min_size(xdgToplevelV6(), size[0], size[1]);
	} else if(xdgToplevel()) {
		xdg_toplevel_set_min_size(xdgToplevel(), size[0], size[1]);
	} else {
		dlg_warn("this wayland wc has no capability for size limits");
	}
}
void WaylandWindowContext::maxSize(nytl::Vec2ui size) {
	if(xdgToplevelV6()) {
		zxdg_toplevel_v6_set_max_size(xdgToplevelV6_.toplevel, size[0], size[1]);
	} else if(xdgToplevel()) {
		xdg_toplevel_set_max_size(xdgToplevel(), size[0], size[1]);
	} else {
		dlg_warn("this wayland wc has no capability for size limits");
	}
}

NativeHandle WaylandWindowContext::nativeHandle() const {
	return {wlSurface_};
}

WindowCapabilities WaylandWindowContext::capabilities() const {
	// return something else when using xdg shell 6
	// make it dependent on the actual shell used
	auto ret = WindowCapability::size |
		WindowCapability::cursor;

	if(customDecorated_) {
		ret |= WindowCapability::customDecoration;
	} else {
		ret |= WindowCapability::serverDecoration;
	}

	if(xdgToplevelV6() || xdgToplevel() || wlShellSurface()) {
		ret |= WindowCapability::fullscreen;
		ret |= WindowCapability::title;
		ret |= WindowCapability::beginMove;
		ret |= WindowCapability::beginResize;
		ret |= WindowCapability::maximize;
	}

	if(xdgToplevelV6() || xdgToplevel()) {
		ret |= WindowCapability::minimize;
		ret |= WindowCapability::sizeLimits;
	}

	return ret;
}

void WaylandWindowContext::customDecorated(bool set) {
	if(set == customDecorated_) {
		return;
	}

	dlg_warn("Can't change decoration mode after initialization");
}

bool WaylandWindowContext::customDecorated() const {
	return customDecorated_;
}

void WaylandWindowContext::maximize() {
	if(wlShellSurface()) {
		wl_shell_surface_set_maximized(wlShellSurface_, nullptr);
	} else if(xdgToplevelV6()) {
		zxdg_toplevel_v6_set_maximized(xdgToplevelV6());
	} else if(xdgToplevel()) {
		xdg_toplevel_unset_maximized(xdgToplevel());
	} else {
		dlg_warn("role cannot be maximized");
	}
}

void WaylandWindowContext::fullscreen() {
	// sicne ny doesn't will with outputs at all, we always pass nullptr
	// as output, leaving the decision to the compositor

	constexpr auto method = WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT;
	if(wlShellSurface()) {
		wl_shell_surface_set_fullscreen(wlShellSurface(), method, 0, nullptr);
	} else if(xdgToplevelV6()) {
		zxdg_toplevel_v6_set_fullscreen(xdgToplevelV6(), nullptr);
	} else if(xdgToplevel()) {
		xdg_toplevel_set_fullscreen(xdgToplevel(), nullptr);
	} else {
		dlg_warn("role does not support fullscreen");
	}
}

void WaylandWindowContext::minimize() {
	if(xdgToplevelV6()) {
		zxdg_toplevel_v6_set_minimized(xdgToplevelV6());
	} else if(xdgToplevel()) {
		xdg_toplevel_set_minimized(xdgToplevel());
	} else if(xdgToplevel()) {
		xdg_toplevel_set_minimized(xdgToplevel());
	} else {
		dlg_warn("role cannot be minimized");
	}
}
void WaylandWindowContext::normalState() {
	if(wlShellSurface()) {
		wl_shell_surface_set_toplevel(wlShellSurface_);
	} else if(xdgToplevelV6()) {
		zxdg_toplevel_v6_unset_fullscreen(xdgToplevelV6());
		zxdg_toplevel_v6_unset_maximized(xdgToplevelV6());
	} else if(xdgToplevel()) {
		xdg_toplevel_unset_fullscreen(xdgToplevel());
		xdg_toplevel_unset_maximized(xdgToplevel());
	} else {
		dlg_warn("invalid surface role");
	}
}
void WaylandWindowContext::beginMove(const EventData* ev) {
	if(!appContext().wlSeat()) {
		dlg_warn("beginMove failed: app context has no seat");
		return;
	}

	auto* data = dynamic_cast<const WaylandEventData*>(ev);
	if(!data || !data->serial) {
		dlg_warn("beginMove failed: invalid event data");
		return;
	}

	if(wlShellSurface()) {
		wl_shell_surface_move(wlShellSurface_, appContext().wlSeat(), data->serial);
	} else if(xdgToplevelV6()) {
		zxdg_toplevel_v6_move(xdgToplevelV6(), appContext().wlSeat(), data->serial);
	} else if(xdgToplevel()) {
		xdg_toplevel_move(xdgToplevel(), appContext().wlSeat(), data->serial);
	} else {
		dlg_warn("role cannot be moved");
	}
}

void WaylandWindowContext::beginResize(const EventData* ev, WindowEdges edge) {
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
	} else if(xdgToplevelV6()) {
		zxdg_toplevel_v6_resize(xdgToplevelV6(), appContext().wlSeat(), serial, wlEdge);
	} else if(xdgToplevel()) {
		xdg_toplevel_resize(xdgToplevel(), appContext().wlSeat(), serial, wlEdge);
	} else {
		dlg_warn("role cannot be resized");
	}
}

void WaylandWindowContext::title(const char* nt) {
	if(wlShellSurface()) {
		wl_shell_surface_set_title(wlShellSurface_, nt);
	} else if(xdgToplevelV6()) {
		zxdg_toplevel_v6_set_title(xdgToplevelV6(), nt);
	} else if(xdgToplevel()) {
		xdg_toplevel_set_title(xdgToplevel(), nt);
	} else {
		dlg_warn("role cannot change title");
	}
}

wl_shell_surface* WaylandWindowContext::wlShellSurface() const {
	if(role_ == WaylandSurfaceRole::shell) {
		return wlShellSurface_;
	}
	return nullptr;
}

zxdg_surface_v6* WaylandWindowContext::xdgSurfaceV6() const {
	if(role_ == WaylandSurfaceRole::xdgToplevelV6) {
		return xdgToplevelV6_.surface;
	}
	return nullptr;
}

zxdg_toplevel_v6* WaylandWindowContext::xdgToplevelV6() const {
	if(role_ == WaylandSurfaceRole::xdgToplevelV6) {
		return xdgToplevelV6_.toplevel;
	}
	return nullptr;
}

xdg_surface* WaylandWindowContext::xdgSurface() const {
	if(role_ == WaylandSurfaceRole::xdgToplevel) {
		return xdgToplevel_.surface;
	}
	return nullptr;
}

xdg_toplevel* WaylandWindowContext::xdgToplevel() const {
	if(role_ == WaylandSurfaceRole::xdgToplevel) {
		return xdgToplevel_.toplevel;
	}
	return nullptr;
}

zxdg_toplevel_decoration_v1* WaylandWindowContext::xdgDecoration() const {
	if(role_ == WaylandSurfaceRole::xdgToplevel) {
		return xdgToplevel_.decoration;
	}
	return nullptr;
}

wl_display& WaylandWindowContext::wlDisplay() const {
	return appContext().wlDisplay();
}

void WaylandWindowContext::attachCommit(wl_buffer* buffer) {
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

Surface WaylandWindowContext::surface() {
	return {};
}

// works for xdg shell v6 and stable
void WaylandWindowContext::reparseState(const wl_array& states) {
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

void WaylandWindowContext::handleFrameCallback(wl_callback*, uint32_t) {
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

void WaylandWindowContext::handleShellSurfacePing(wl_shell_surface*, uint32_t serial) {
	wl_shell_surface_pong(wlShellSurface(), serial);
}

void WaylandWindowContext::handleShellSurfaceConfigure(wl_shell_surface*,
		uint32_t edges, int32_t width, int32_t height) {
	nytl::unused(edges);
	size_[0] = width;
	size_[1] = height;

	if(!pendingResize_) {
		pendingResize_ = true;
		appContext().deferred.add([&]{
			pendingResize_ = false;
			SizeEvent se;
			se.size = size_;
			listener().resize(se);
			refresh();
		}, this);
	}
}

void WaylandWindowContext::handleShellSurfacePopupDone(wl_shell_surface*) {
	// ignore, we don't use popups
}

void WaylandWindowContext::handleXdgSurfaceV6Configure(zxdg_surface_v6*,
		uint32_t serial) {
	xdgToplevelV6_.configured = true;
	if(pendingResize_ == 0) {
		appContext().deferred.add([&]{
			zxdg_surface_v6_ack_configure(xdgSurfaceV6(), pendingResize_);
			pendingResize_ = 0;
			SizeEvent se;
			se.size = size_;
			listener().resize(se);
			refresh();
		}, this);
	}
	pendingResize_ = serial;
}

void WaylandWindowContext::handleXdgToplevelV6Configure(zxdg_toplevel_v6*,
		int32_t width, int32_t height, wl_array* states) {
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

void WaylandWindowContext::handleXdgToplevelV6Close(zxdg_toplevel_v6*) {
	CloseEvent ce;
	listener().close(ce);
}

void WaylandWindowContext::handleXdgSurfaceConfigure(xdg_surface*,
		uint32_t serial) {
	xdgToplevel_.configured = true;
	if(pendingResize_ == 0) {
		appContext().deferred.add([&]{
			xdg_surface_ack_configure(xdgSurface(), pendingResize_);
			pendingResize_ = 0;
			SizeEvent se;
			se.size = size_;
			listener().resize(se);
			refresh();
		}, this);
	}
	pendingResize_ = serial;
}

void WaylandWindowContext::handleXdgToplevelConfigure(xdg_toplevel*,
		int32_t width, int32_t height, wl_array* states) {
	reparseState(*states);
	if(!width || !height) {
		// in this case we simply keep the (copied from WindowSettings)
		// width and height
		return;
	}

	// we store the new size but not yet actually resize/redraw the window
	// this should/can only be done after we recevie the surfaceConfigure event
	// we will also then send the size event
	size_[0] = width;
	size_[1] = height;
}

void WaylandWindowContext::handleXdgToplevelClose(xdg_toplevel*) {
	CloseEvent ce;
	listener().close(ce);
}

void WaylandWindowContext::handleXdgDecorationConfigure(
		zxdg_toplevel_decoration_v1*, uint32_t mode) {
	customDecorated_ = (mode == ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
}

} // namespace ny
