// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/wayland/include.hpp>
#include <ny/wayland/util.hpp> // ny::wayland::ShmBuffer

#include <ny/windowContext.hpp> // ny::WindowContexts
#include <ny/windowSettings.hpp> // ny::WindowSettings
#include <nytl/vec.hpp> // nytl::Vec

namespace ny {

/// Specifies the different roles a WaylandWindowContext can have.
enum class WaylandSurfaceRole : unsigned int {
	none,
	shell,
	xdgToplevelV6,
	xdgToplevel,
};

/// WindowSettings class for wayland WindowContexts.
class WaylandWindowSettings : public WindowSettings {};

/// Wayland WindowContext implementation.
/// Basically holds a wayland surface with a description on how it is used.
class WaylandWindowContext : public WindowContext {
public:
	using Role = WaylandSurfaceRole;

public:
	WaylandWindowContext(WaylandAppContext&, const WaylandWindowSettings& = {});
	virtual ~WaylandWindowContext();

	void refresh() override;
	void show() override;
	void hide() override;

	void minSize(nytl::Vec2ui minSize) override;
	void maxSize(nytl::Vec2ui maxSize) override;

	void size(nytl::Vec2ui minSize) override;
	void position(nytl::Vec2i position) override;

	void cursor(const Cursor& c) override;
	NativeHandle nativeHandle() const override;

	WindowCapabilities capabilities() const override;
	Surface surface() override;

	//toplevel
	void maximize() override;
	void minimize() override;
	void fullscreen() override;
	void normalState() override;

	void beginMove(const EventData* ev) override;
	void beginResize(const EventData* event, WindowEdges edges) override;

	void title(const char* name) override;
	void icon(const Image&) override {}

	void customDecorated(bool) override {}
	bool customDecorated() const override { return true; }

	// - wayland specific -
	wl_surface& wlSurface() const { return *wlSurface_; };
	wl_callback* frameCallback() const { return frameCallback_; }
	WaylandSurfaceRole surfaceRole() const { return role_; }
	nytl::Vec2ui size() const { return size_; }

	//return nullptr if this object has another role
	wl_shell_surface* wlShellSurface() const;
	wl_subsurface* wlSubsurface() const;

	zxdg_surface_v6* xdgSurfaceV6() const;
	zxdg_toplevel_v6* xdgToplevelV6() const;

	xdg_surface* xdgSurface() const;
	xdg_toplevel* xdgToplevel() const;

	wl_buffer* wlCursorBuffer() const { return cursorBuffer_; }
	nytl::Vec2i cursorHotspot() const { return cursorHotspot_; }
	nytl::Vec2ui cursorSize() const { return cursorSize_; }

	/// Attaches the given buffer, damages the surface and commits it.
	/// Does also add a frameCallback to the surface.
	/// If called with a nullptr, no framecallback will be attached and the surface will
	/// be unmapped. Note that if the WindowContext is currently hidden or not mapped,
	/// no buffer will be attached.
	void attachCommit(wl_buffer* buffer);

	WaylandAppContext& appContext() const { return *appContext_; }
	wl_display& wlDisplay() const;

protected:

	/// Tries to reparse the current state from the array of xdg states.
	/// Will send a StateEvent if it changed
	void reparseState(const wl_array& states);

	// init helpers
	void createShellSurface(const WaylandWindowSettings& ws);
	void createXdgToplevelV6(const WaylandWindowSettings& ws);
	void createXdgToplevel(const WaylandWindowSettings& ws);

	// listeners
	void handleFrameCallback(wl_callback*, uint32_t);
	void handleShellSurfacePing(wl_shell_surface*, uint32_t);
	void handleShellSurfaceConfigure(wl_shell_surface*, uint32_t, int32_t, int32_t);
	void handleShellSurfacePopupDone(wl_shell_surface*);

	void handleXdgSurfaceV6Configure(zxdg_surface_v6*, uint32_t);
	void handleXdgToplevelV6Configure(zxdg_toplevel_v6*, int32_t, int32_t, wl_array*);
	void handleXdgToplevelV6Close(zxdg_toplevel_v6*);

	void handleXdgSurfaceConfigure(xdg_surface*, uint32_t);
	void handleXdgToplevelConfigure(xdg_toplevel*, int32_t, int32_t, wl_array*);
	void handleXdgToplevelClose(xdg_toplevel*);

protected:
	WaylandAppContext* appContext_ {};
	wl_surface* wlSurface_ {};
	nytl::Vec2ui size_ {};

	// if this is == nullptr, the window is ready to be redrawn.
	// otherwise waiting for the callback to be called
	wl_callback* frameCallback_ {};

	// stores if the window has a pending refresh request, i.e. if it should refresh
	// as soon as possible
	// flag will be set by refresh() and trigger a DrawEvent when frameEvent is called
	bool refreshFlag_ {};
	bool refreshPending_ {}; // for deferred refresh

	// stores which kinds of surface this context holds
	WaylandSurfaceRole role_ = WaylandSurfaceRole::none;

	// state for xdgToplevelV6
	struct XdgToplevelV6 {
		zxdg_surface_v6* surface;
		zxdg_toplevel_v6* toplevel;
		bool configured;
	};

	// state for xdgToplevel role
	struct XdgToplevel {
		xdg_surface* surface;
		xdg_toplevel* toplevel;
		bool configured;
	};

	// the different surface roles this surface can have.
	// the union will be activated depending on role_
	// xdg roles are preferred over the plain wl ones if available
	union {
		wl_shell_surface* wlShellSurface_ = nullptr;
		XdgToplevelV6 xdgToplevelV6_;
		XdgToplevel xdgToplevel_;
	};

	// current toplevel state for xdg toplevel windows
	ToplevelState currentXdgState_ {ToplevelState::normal};

	wayland::ShmBuffer shmCursorBuffer_ {}; // only needed when cursor is custom image

	wl_buffer* cursorBuffer_ {};
	nytl::Vec2i cursorHotspot_ {};
	nytl::Vec2ui cursorSize_ {};

	// whether there is a pending deferred resize event
	bool pendingResize_ {};
};

} // namespace ny
