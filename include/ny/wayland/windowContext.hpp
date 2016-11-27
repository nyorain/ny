// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/wayland/include.hpp>
#include <ny/wayland/util.hpp>

#include <ny/windowContext.hpp>
#include <ny/windowSettings.hpp>
#include <nytl/vec.hpp>

namespace ny
{

///Specifies the different roles a WaylandWindowContext can have.
enum class WaylandSurfaceRole : unsigned int
{
    none,
    shell,
    sub,
    xdgSurfaceV5,
    xdgPopupV5,
	xdgToplevelV6,
	xdgPopupV6
};

///WindowSettings class for wayland WindowContexts.
class WaylandWindowSettings : public WindowSettings {};

///Wayland WindowContext implementation.
///Basically holds a wayland surface with a description on how it is used.
class WaylandWindowContext : public WindowContext
{
public:
	using Role = WaylandSurfaceRole;

public:
    WaylandWindowContext(WaylandAppContext& ac, const WaylandWindowSettings& s = {});
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

    void title(nytl::StringParam name) override;
	void icon(const ImageData&) override {}

	bool customDecorated() const override { return true; }
	void addWindowHints(WindowHints hints) override;
	void removeWindowHints(WindowHints hints) override;

    //wayland specific functions
    wl_surface& wlSurface() const { return *wlSurface_; };
	wl_callback* frameCallback() const { return frameCallback_; }
	WaylandSurfaceRole surfaceRole() const { return role_; }
	nytl::Vec2ui size() const { return size_; }
	bool shown() const { return shown_; }

	//return nullptr if this object has another role
    wl_shell_surface* wlShellSurface() const;
    wl_subsurface* wlSubsurface() const;
    xdg_surface* xdgSurfaceV5() const;
    xdg_popup* xdgPopupV5() const;

	zxdg_surface_v6* xdgSurfaceV6() const;
	zxdg_popup_v6* xdgPopupV6() const;
	zxdg_toplevel_v6* xdgToplevelV6() const;

	wl_buffer* wlCursorBuffer() const { return cursorBuffer_; }
	nytl::Vec2i cursorHotspot() const { return cursorHotspot_; }
	nytl::Vec2ui cursorSize() const { return cursorSize_; }

	///Attaches the given buffer, damages the surface and commits it.
	///Does also add a frameCallback to the surface.
	///If called with a nullptr, no framecallback will be attached and the surface will
	///be unmapped. Note that if the WindowContext is hidden, no buffer will be
	///attached.
	void attachCommit(wl_buffer* buffer);

	WaylandAppContext& appContext() const { return *appContext_; }
	wl_display& wlDisplay() const;

protected:
	//helpers
    void createShellSurface(const WaylandWindowSettings& ws);
    void createXdgSurfaceV5(const WaylandWindowSettings& ws);
    void createXdgSurfaceV6(const WaylandWindowSettings& ws);
    void createSubsurface(wl_surface& parent, const WaylandWindowSettings& ws);

	//listeners
	void handleFrameCallback();
	void handleShellSurfacePing(unsigned int serial);
	void handleShellSurfaceConfigure(unsigned int edges, int width, int height);
	void handleShellSurfacePopupDone();

	void handleXdgSurfaceV5Configure(int width, int height, wl_array* states, unsigned int serial);
	void handleXdgSurfaceV5Close();
	void handleXdgPopupV5Done();

	void handleXdgSurfaceV6Configure(unsigned int serial);
	void handleXdgToplevelV6Configure(int width, int height, wl_array* states);
	void handleXdgToplevelV6Close();
	void handleXdgPopupV6Configure(int x, int y, int width, int height);
	void handleXdgPopupV6Done();

protected:
	WaylandAppContext* appContext_ {};
    wl_surface* wlSurface_ {};
	nytl::Vec2ui size_ {};

	//if this is == nullptr, the window is ready to be redrawn.
	//otherwise waiting for the callback to be called
    wl_callback* frameCallback_ {};

	//stores if the window has a pending refresh request, i.e. if it should refresh
	//as soon as possible
	//flag will be set by refresh() and trigger a DrawEvent when frameEvent is called
    bool refreshFlag_ = false;

    //stores which kinds of surface this context holds
	WaylandSurfaceRole role_ = WaylandSurfaceRole::none;

	//the different surface roles this surface can have.
	//the union will be activated depending on role_
	//xdg roles are preferred over the plain wl ones if available
    union
    {
        wl_shell_surface* wlShellSurface_ = nullptr;
        wl_subsurface* wlSubsurface_;
        xdg_surface* xdgSurfaceV5_;
        xdg_popup* xdgPopupV5_;

		struct
		{
			zxdg_surface_v6* xdgSurfaceV6_;
			bool xdgV6Configured_;

			union
			{
				zxdg_toplevel_v6* xdgToplevelV6_;
				zxdg_popup_v6* xdgPopupV6_;
			};
		};
    };

	bool shown_ {}; //Whether the WindowContext should be shown or hidden

	wayland::ShmBuffer shmCursorBuffer_ {}; //only needed when cursor is custom image

	wl_buffer* cursorBuffer_ {};
	nytl::Vec2i cursorHotspot_ {};
	nytl::Vec2ui cursorSize_ {};
};


}
