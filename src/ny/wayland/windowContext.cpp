// Copyright (c) 2017 nyorain
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
#include <ny/log.hpp>

#include <nytl/vecOps.hpp>

#include <wayland-cursor.h>

#include <iostream>
#include <cstring>

// TODO: correct xdg surface configure/ sizing, better subsurface support
// TODO: implement show capability? could be done with custom egl surface, show flag
//	and custom refrsh/redraw handling (see earlier commits)

namespace ny {

WaylandWindowContext::WaylandWindowContext(WaylandAppContext& ac,
	const WaylandWindowSettings& settings) : appContext_(&ac)
{
	// parse settings
	size_ = settings.size;
	if(size_ == defaultSize) size_ = fallbackSize;
	if(settings.listener) listener(*settings.listener);

	// surface
	if(settings.nativeHandle) {
		// In this case we simply copy the surface and not create/handle any role for it

		// TODO: use WaylandWindowSettings for a role on a provided handle
		// then no custom role has to be created here
		// At the moement this constructor crashs with native handle that already has a role...
		wlSurface_ = settings.nativeHandle.asPtr<wl_surface>();
	} else {
		wlSurface_ = wl_compositor_create_surface(&ac.wlCompositor());
		if(!wlSurface_)
			throw std::runtime_error("ny::WaylandWindowContext: could not create wl_surface");
		wl_surface_set_user_data(wlSurface_, this);
	}

	if(settings.parent.pointer()) {
		auto& parent = *reinterpret_cast<wl_surface*>(settings.parent.pointer());
		createSubsurface(parent, settings);
	} else {
		if(ac.xdgShellV6()) createXdgSurfaceV6(settings);
		else if(ac.xdgShellV5()) createXdgSurfaceV5(settings);
		else if(ac.wlShell()) createShellSurface(settings);
		else throw std::runtime_error("ny::WaylandWindowContext: compositor has no shell global");

		if(!settings.title.empty()) title(settings.title);
		switch(settings.initState) {
			case ToplevelState::normal: normalState(); break;
			case ToplevelState::fullscreen: fullscreen(); break;
			case ToplevelState::maximized: maximize(); break;
			case ToplevelState::minimized: minimize(); break;
			default: break;
		}
	}

	cursor(settings.cursor);
	if(settings.show) show();
}

WaylandWindowContext::~WaylandWindowContext()
{
	if(frameCallback_) wl_callback_destroy(frameCallback_);

	// role
	if(wlShellSurface()) wl_shell_surface_destroy(wlShellSurface_);
	else if(wlSubsurface()) wl_subsurface_destroy(wlSubsurface_);
	else if(xdgSurfaceV5()) xdg_surface_destroy(xdgSurfaceV5_);
	else if(xdgPopupV5()) xdg_popup_destroy(xdgPopupV5_);
	else if(xdgSurfaceV6()) {
		if(xdgPopupV6()) zxdg_popup_v6_destroy(xdgSurfaceV6_.popup);
		else if(xdgToplevelV6()) zxdg_toplevel_v6_destroy(xdgSurfaceV6_.toplevel);
		zxdg_surface_v6_destroy(xdgSurfaceV6_.surface);
	}

	if(wlSurface_) wl_surface_destroy(wlSurface_);
}

void WaylandWindowContext::createShellSurface(const WaylandWindowSettings& ws)
{
	using WWC = WaylandWindowContext;
	constexpr static wl_shell_surface_listener shellSurfaceListener = {
		memberCallback<decltype(&WWC::handleShellSurfacePing), &WWC::handleShellSurfacePing>,
		memberCallback<decltype(&WWC::handleShellSurfaceConfigure),
			&WWC::handleShellSurfaceConfigure>,
		memberCallback<decltype(&WWC::handleShellSurfacePopupDone),
			&WWC::handleShellSurfacePopupDone>
	};

	wlShellSurface_ = wl_shell_get_shell_surface(appContext().wlShell(), wlSurface_);
	if(!wlShellSurface_)
		throw std::runtime_error("ny::WaylandWindowContext: failed to create wl_shell_surface");

	role_ = WaylandSurfaceRole::shell;

	wl_shell_surface_set_user_data(wlShellSurface_, this);
	wl_shell_surface_add_listener(wlShellSurface_, &shellSurfaceListener, this);

	wl_shell_surface_set_title(wlShellSurface_, ws.title.c_str());
	wl_shell_surface_set_class(wlShellSurface_, "ny::application"); //TODO: AppContextSettings
}

void WaylandWindowContext::createXdgSurfaceV5(const WaylandWindowSettings& ws)
{
	using WWC = WaylandWindowContext;
	constexpr static xdg_surface_listener xdgSurfaceListener = {
		memberCallback<decltype(&WWC::handleXdgSurfaceV5Configure),
			&WWC::handleXdgSurfaceV5Configure>,
		memberCallback<decltype(&WWC::handleXdgSurfaceV5Close),
			&WWC::handleXdgSurfaceV5Close>
	};

	xdgSurfaceV5_ = xdg_shell_get_xdg_surface(appContext().xdgShellV5(), wlSurface_);
	if(!xdgSurfaceV5_)
		throw std::runtime_error("ny::WaylandWindowContext: failed to create xdg_surface v5");

	role_ = WaylandSurfaceRole::xdgSurfaceV5;

	// xdg_surface_set_window_geometry(xdgSurfaceV5_, 0, 0, size_.x, size_.y);
	xdg_surface_set_user_data(xdgSurfaceV5_, this);
	xdg_surface_add_listener(xdgSurfaceV5_, &xdgSurfaceListener, this);

	xdg_surface_set_title(xdgSurfaceV5_, ws.title.c_str());
	xdg_surface_set_app_id(xdgSurfaceV5_, appContext().appName());
}

void WaylandWindowContext::createXdgSurfaceV6(const WaylandWindowSettings& ws)
{
	using WWC = WaylandWindowContext;
	constexpr static zxdg_surface_v6_listener xdgSurfaceListener = {
		memberCallback<decltype(&WWC::handleXdgSurfaceV6Configure),
			&WWC::handleXdgSurfaceV6Configure>
	};

	constexpr static zxdg_toplevel_v6_listener xdgToplevelListener = {
		memberCallback<decltype(&WWC::handleXdgToplevelV6Configure),
			&WWC::handleXdgToplevelV6Configure>,
		memberCallback<decltype(&WWC::handleXdgToplevelV6Close),
			&WWC::handleXdgToplevelV6Close>
	};

	// create the xdg surface
	xdgSurfaceV6_.surface = zxdg_shell_v6_get_xdg_surface(appContext().xdgShellV6(), wlSurface_);
	if(!xdgSurfaceV6_.surface)
		throw std::runtime_error("ny::WaylandWindowContext: failed to create xdg_surface v6");

	// create the xdg toplevel for the surface
	xdgSurfaceV6_.toplevel = zxdg_surface_v6_get_toplevel(xdgSurfaceV6_.surface);
	if(!xdgSurfaceV6_.toplevel)
		throw std::runtime_error("ny::WaylandWindowContext: failed to create xdg_toplevel v6");

	// commit to apply the role
	wl_surface_commit(wlSurface_);

	role_ = WaylandSurfaceRole::xdgToplevelV6;
	xdgSurfaceV6_.configured = false;

	// TODO: when we use this here we override the size our WindowContext would get i.e. on
	// a tiling window manager which is not what we want. But otherwise if the compositor
	// wants the application to choose the size we need this call to actually choose it.
	// how to handle this? somehow parse the configure events we get and react to it
	// zxdg_surface_v6_set_window_geometry(xdgSurfaceV6_.surface, 0, 0, size_.x, size_.y);

	zxdg_surface_v6_add_listener(xdgSurfaceV6_.surface, &xdgSurfaceListener, this);
	zxdg_toplevel_v6_add_listener(xdgSurfaceV6_.toplevel, &xdgToplevelListener, this);

	zxdg_toplevel_v6_set_title(xdgSurfaceV6_.toplevel, ws.title.c_str());
	zxdg_toplevel_v6_set_app_id(xdgSurfaceV6_.toplevel, appContext().appName());
}

void WaylandWindowContext::createXdgPopupV5(const WaylandWindowSettings& ws)
{
	// TODO! - cannot be used yet

	using WWC = WaylandWindowContext;
	constexpr static xdg_popup_listener xdgPopupListener = {
		memberCallback<decltype(&WWC::handleXdgPopupV5Done), &WWC::handleXdgPopupV5Done,
			void(xdg_popup*)>
	};

	wl_surface* parent {};
	nytl::Vec2i position = ws.position;
	unsigned int serial {};
	if(position == defaultPosition) position = fallbackPosition;
	xdgPopupV5_ = xdg_shell_get_xdg_popup(appContext().xdgShellV5(), &wlSurface(), parent,
		appContext().wlSeat(), serial, position.x, position.y);

	if(!xdgPopupV5_)
		throw std::runtime_error("ny::WaylandWindowContext: failed to create xdg_popup v5");

	role_ = WaylandSurfaceRole::xdgPopupV5;

	xdg_popup_add_listener(xdgPopupV5_, &xdgPopupListener, this);
}

void WaylandWindowContext::createXdgPopupV6(const WaylandWindowSettings& ws)
{
	// TODO! - cannot be used yet

	using WWC = WaylandWindowContext;
	constexpr static zxdg_surface_v6_listener xdgSurfaceListener = {
		memberCallback<decltype(&WWC::handleXdgSurfaceV6Configure),
			&WWC::handleXdgSurfaceV6Configure>
	};

	constexpr static zxdg_popup_v6_listener xdgPopupListener = {
		memberCallback<decltype(&WWC::handleXdgPopupV6Configure), &WWC::handleXdgPopupV6Configure>,
		memberCallback<decltype(&WWC::handleXdgPopupV6Done), &WWC::handleXdgPopupV6Done>
	};

	// create the xdg surface
	xdgSurfaceV6_.surface = zxdg_shell_v6_get_xdg_surface(appContext().xdgShellV6(), wlSurface_);
	if(!xdgSurfaceV6_.surface)
		throw std::runtime_error("ny::WaylandWindowContext: failed to create xdg_surface v6");

	// create the popup role
	wl_surface* parent {};
	nytl::Vec2i position = ws.position;
	if(position == defaultPosition) position = fallbackPosition;
	unsigned int serial {};

	xdgPopupV5_ = xdg_shell_get_xdg_popup(appContext().xdgShellV5(), &wlSurface(), parent,
		appContext().wlSeat(), serial, position.x, position.y);

	if(!xdgSurfaceV6_.popup)
		throw std::runtime_error("ny::WaylandWindowContext: failed to create xdg_popup v5");

	role_ = WaylandSurfaceRole::xdgPopupV6;

	zxdg_surface_v6_add_listener(xdgSurfaceV6_.surface, &xdgSurfaceListener, this);
	zxdg_popup_v6_add_listener(xdgSurfaceV6_.popup, &xdgPopupListener, this);
}

void WaylandWindowContext::createSubsurface(wl_surface& parent, const WaylandWindowSettings&)
{
	auto subcomp = appContext().wlSubcompositor();
	if(!subcomp) throw std::runtime_error("ny::WaylandWindowContext: no wl_subcompositor");

	role_ = WaylandSurfaceRole::sub;
	wlSubsurface_ = wl_subcompositor_get_subsurface(subcomp, wlSurface_, &parent);

	if(!wlSubsurface_)
		throw std::runtime_error("ny::WaylandWindowContext: failed to create wl_subsurface");

	wl_subsurface_set_user_data(wlSubsurface_, this);
	wl_subsurface_set_desync(wlSubsurface_);
}

void WaylandWindowContext::refresh()
{
	// if there is an active frameCallback just set the flag that we want to refresh
	// as soon as possible
	if(frameCallback_ || !xdgSurfaceV6_.configured) {
		refreshFlag_ = true;
		return;
	}

	// otherwise send a draw event.
	DrawEvent de {};
	listener().draw(de);
}

void WaylandWindowContext::show()
{
	warning("ny::WaylandWindowContext::show: not supported");
}

void WaylandWindowContext::hide()
{
	warning("ny::WaylandWindowContext::hide: not supported");
}

void WaylandWindowContext::size(nytl::Vec2ui size)
{
	size_ = size;
	refresh();
}

void WaylandWindowContext::position(nytl::Vec2i position)
{
	// TODO: support xdg v6 positioner
	if(wlSubsurface()) {
		wl_subsurface_set_position(wlSubsurface_, position.x, position.y);
	} else {
		warning("ny::WaylandWindowContext::position: wayland does not support custom positions");
	}
}

void WaylandWindowContext::cursor(const Cursor& cursor)
{
	auto wmc = appContext().waylandMouseContext();
	if(!wmc) {
		warning("ny::WaylandWindowContext::cursor: no WaylandMouseContext");
		return;
	}

	if(cursor.type() == Cursor::Type::image) {
		auto img = cursor.image();
		if(!img.data) {
			warning("ny::WaylandWindowContext::cursor: invalid image cursor");
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
		auto cursorTheme = appContext().wlCursorTheme();
		auto* wlCursor = wl_cursor_theme_get_cursor(cursorTheme, cursorName);
		if(!wlCursor) {
			warning("ny::WaylandWindowContext::cursor: failed to retrieve cursor ", cursorName);
			return;
		}

		// TODO: handle mulitple/animated images
		auto img = wlCursor->images[0];

		cursorBuffer_ = wl_cursor_image_get_buffer(img);

		cursorHotspot_.x = img->hotspot_y;
		cursorHotspot_.x = img->hotspot_x;

		cursorSize_.x = img->width;
		cursorSize_.y = img->height;
	}

	// update the cursor if needed
	if(wmc->over() == this)
		wmc->cursorBuffer(cursorBuffer_, cursorHotspot_, cursorSize_);
}

void WaylandWindowContext::minSize(nytl::Vec2ui size)
{
	if(xdgToplevelV6()) zxdg_toplevel_v6_set_min_size(xdgSurfaceV6_.toplevel, size[0], size[1]);
	else warning("ny::WaylandWindowContext::maxSize: wayland has no capability for size limits");
}
void WaylandWindowContext::maxSize(nytl::Vec2ui size)
{
	if(xdgToplevelV6()) zxdg_toplevel_v6_set_max_size(xdgSurfaceV6_.toplevel, size[0], size[1]);
	else warning("ny::WaylandWindowContext::maxSize: wayland has no capability for size limits");
}

NativeHandle WaylandWindowContext::nativeHandle() const
{
	// TODO: somehow also return window role?
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
	else warning("ny::WaylandWindowContext::maximize: role cannot be maximized");
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
	else warning("ny::WaylandWindowContext::fullscreen: role does not support fullscreen");
}

void WaylandWindowContext::minimize()
{
	if(xdgSurfaceV5()) xdg_surface_set_minimized(xdgSurfaceV5_);
	else if(xdgToplevelV6()) zxdg_toplevel_v6_set_minimized(xdgSurfaceV6_.toplevel);
	else warning("ny::WaylandWindowContext::minimize: role cannot be minimized");
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
		warning("ny::WaylandWindowContext::normalState: invalid surface role");
	}
}
void WaylandWindowContext::beginMove(const EventData* ev)
{
	if(!appContext().wlSeat()) {
		warning("ny::WaylandWindowContext:beginMove: failed: app context has no seat");
		return;
	}

	auto* data = dynamic_cast<const WaylandEventData*>(ev);
	if(!data || !data->serial) {
		warning("ny::WaylandWindowContext::beginMove: failed: invalid event data");
		return;
	}

	if(wlShellSurface())
		wl_shell_surface_move(wlShellSurface_, appContext().wlSeat(), data->serial);
	else if(xdgSurfaceV5())
		xdg_surface_move(xdgSurfaceV5_, appContext().wlSeat(), data->serial);
	else if(xdgToplevelV6())
		zxdg_toplevel_v6_move(xdgSurfaceV6_.toplevel, appContext().wlSeat(), data->serial);
	else warning("ny::WaylandWindowContext::beginMove: role cannot be moved");
}

void WaylandWindowContext::beginResize(const EventData* ev, WindowEdges edge)
{
	if(!appContext().wlSeat()) {
		warning("ny::WaylandWindowContext:beginResize failed: app context has no seat");
		return;
	}

	auto* data = dynamic_cast<const WaylandEventData*>(ev);
	if(!data || !data->serial) {
		warning("ny::WaylandWindowContext::beginResize failed: invalid event data");
		return;
	}

	// wayland core and the xdg protocols have the same edge values
	auto wlEdge = edge.value();
	auto serial = data->serial;

	if(!wlEdge) {
		warning("ny::WaylandWindowContext::beginReisze: no/invalid edge given");
		return;
	}

	if(wlShellSurface())
		wl_shell_surface_resize(wlShellSurface_, appContext().wlSeat(), serial, wlEdge);
	else if(xdgSurfaceV5())
		xdg_surface_resize(xdgSurfaceV5_, appContext().wlSeat(), serial, wlEdge);
	else if(xdgToplevelV6())
		zxdg_toplevel_v6_resize(xdgSurfaceV6_.toplevel, appContext().wlSeat(), serial, wlEdge);
	else warning("ny::WaylandWindowContext::beginResize: role cannot be resized");
}

void WaylandWindowContext::title(nytl::StringParam titlestring)
{
	if(wlShellSurface()) wl_shell_surface_set_title(wlShellSurface_, titlestring);
	else if(xdgSurfaceV5()) xdg_surface_set_title(xdgSurfaceV5_, titlestring);
	else if(xdgToplevelV6()) zxdg_toplevel_v6_set_title(xdgSurfaceV6_.toplevel, titlestring);
	else warning("ny::WaylandWindowContext::title: role cannot change title");
}

wl_shell_surface* WaylandWindowContext::wlShellSurface() const
{
	return (role_ == WaylandSurfaceRole::shell) ? wlShellSurface_ : nullptr;
}

wl_subsurface* WaylandWindowContext::wlSubsurface() const
{
	return (role_ == WaylandSurfaceRole::sub) ? wlSubsurface_ : nullptr;
}

xdg_surface* WaylandWindowContext::xdgSurfaceV5() const
{
	return (role_ == WaylandSurfaceRole::xdgSurfaceV5) ? xdgSurfaceV5_ : nullptr;
}

xdg_popup* WaylandWindowContext::xdgPopupV5() const
{
	return (role_ == WaylandSurfaceRole::xdgPopupV5) ? xdgPopupV5_ : nullptr;
}

zxdg_surface_v6* WaylandWindowContext::xdgSurfaceV6() const
{
	if(role_ == WaylandSurfaceRole::xdgToplevelV6 || role_ == WaylandSurfaceRole::xdgPopupV6)
	return xdgSurfaceV6_.surface;

	return nullptr;
}

zxdg_popup_v6* WaylandWindowContext::xdgPopupV6() const
{
	return (role_ == WaylandSurfaceRole::xdgPopupV6) ? xdgSurfaceV6_.popup : nullptr;
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
		memberCallback<decltype(&WWC::handleFrameCallback), &WWC::handleFrameCallback>
	};

	frameCallback_ = wl_surface_frame(wlSurface_);
	wl_callback_add_listener(frameCallback_, &frameListener, this);
	wl_surface_damage(wlSurface_, 0, 0, size_.x, size_.y);
	wl_surface_attach(wlSurface_, buffer, 0, 0);

	wl_surface_commit(wlSurface_);
}

Surface WaylandWindowContext::surface()
{
	return {};
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
	nytl::unused(edges); // TODO

	auto newSize = nytl::Vec2ui(width, height);

	SizeEvent se;
	se.size = newSize;
	listener().resize(se);

	size(newSize);
}

void WaylandWindowContext::handleShellSurfacePopupDone(wl_shell_surface*)
{
	// TODO
}

void WaylandWindowContext::handleXdgSurfaceV5Configure(xdg_surface*, int32_t width, int32_t height,
	wl_array* states, uint32_t serial)
{
	// TODO
	if(!width || !height) return;
	nytl::unused(states);

	auto newSize = nytl::Vec2ui(width, height);

	SizeEvent se;
	se.size = newSize;
	listener().resize(se);

	xdg_surface_ack_configure(xdgSurfaceV5(), serial);
	size(newSize);
}

void WaylandWindowContext::handleXdgSurfaceV5Close(xdg_surface*)
{
	CloseEvent ce;
	listener().close(ce);
}

void WaylandWindowContext::handleXdgPopupV5Done(xdg_popup*)
{
	// TODO: we must close the popup... any way to do/signal that
	CloseEvent ce;
	listener().close(ce);
}

void WaylandWindowContext::handleXdgSurfaceV6Configure(zxdg_surface_v6*, uint32_t serial)
{
	// TODO: somehow check if size has really changed.
	// if not, dont send size event and redraw
	xdgSurfaceV6_.configured = true;
	zxdg_surface_v6_ack_configure(xdgSurfaceV6(), serial);

	SizeEvent se;
	se.size = size_;;
	listener().resize(se);

	refresh();
}

void WaylandWindowContext::handleXdgToplevelV6Configure(zxdg_toplevel_v6*, int32_t width,
	int32_t height, wl_array* states)
{
	nytl::unused(states);
	if(!width || !height) return;

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

void WaylandWindowContext::handleXdgPopupV6Configure(zxdg_popup_v6*, int32_t x, int32_t y,
	int32_t width, int32_t height)
{
	if(!width || !height) return;
	nytl::unused(x, y);

	auto newSize = nytl::Vec2ui(width, height);

	SizeEvent se;
	se.size = newSize;
	listener().resize(se);

	size(newSize);
}

void WaylandWindowContext::handleXdgPopupV6Done(zxdg_popup_v6*)
{
	// TODO: we must close the popup... any way to do/signal that
	CloseEvent ce;
	listener().close(ce);
}

} // namespace ny
