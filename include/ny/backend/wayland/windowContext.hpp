#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/windowContext.hpp>
#include <ny/backend/windowSettings.hpp>

namespace ny
{

///Specifies the different roles a WaylandWindowContext can have.
enum class WaylandSurfaceRole
{
    none,
    shell,
    sub,
    xdg,
    xdgPopup
};

///WindowSettings class for wayland WindowContexts.
class WaylandWindowSettings : public WindowSettings {};


///Wayland WindowContext implementation.
///Basically holds a wayland surface with a description on how it is used.
class WaylandWindowContext : public WindowContext
{
public:
    WaylandWindowContext(WaylandAppContext& ac, const WaylandWindowSettings& s = {});
    virtual ~WaylandWindowContext();

    void refresh() override;
	DrawGuard draw() override;

    void show() override;
    void hide() override;

    void droppable(const DataTypes&) override;

    void minSize(const Vec2ui&) override;
    void maxSize(const Vec2ui&) override;

    void size(const Vec2ui& size) override;
    void position(const Vec2i& position) override;

    void cursor(const Cursor& c) override;
	NativeWindowHandle nativeHandle() const override;

	WindowCapabilities capabilities() const override { return {}; }

    //toplevel
    void maximize() override;
    void minimize() override;
    void fullscreen() override;
    void normalState() override;

    void beginMove(const MouseButtonEvent* ev) override;
    void beginResize(const MouseButtonEvent* event, WindowEdges edges) override;

    void title(const std::string& name) override;
	void icon(const ImageData*) override {}

	bool customDecorated() const override { return true; }
	void addWindowHints(WindowHints hints) override;
	void removeWindowHints(WindowHints hints) override;

	//
	bool handleEvent(const Event& event) override;


    //wayland specific functions
    wl_surface& wlSurface() const { return *wlSurface_; };
	wl_callback* frameCallback() const { return frameCallback_; }
	WaylandSurfaceRole surfaceRole() const { return role_; }

    wl_shell_surface* wlShellSurface() const; 
    wl_subsurface* wlSubsurface() const; 
    xdg_surface* xdgSurface() const; 
    xdg_popup* xdgPopup() const; 

	WaylandAppContext& appContext() const { return *appContext_; }
	wl_display& wlDisplay() const;

protected:
    //util functions
    void createShellSurface();
    void createXDGSurface();
    void createXDGPopup();
    void createSubsurface(wl_surface& parent);

protected:
	WaylandAppContext* appContext_ = nullptr;
    wl_surface* wlSurface_ = nullptr;
	
	//if this is == nullptr, the window is ready to be redrawn.
	//otherwise waiting for the callback to be called
    wl_callback* frameCallback_ = nullptr; 

	//stores if the window has a pending refresh request, i.e. if it should refresh
	//as soon as possible
    bool refreshFlag_ = 0;

    //stores which kinds of surface this context holds
	WaylandSurfaceRole role_ = WaylandSurfaceRole::none;

	//the different surface roles this surface can have.
	//the union will be activated depending on role_
    union
    {
        wl_shell_surface* wlShellSurface_ = nullptr;
        xdg_surface* xdgSurface_;
        xdg_popup* xdgPopup_;
        wl_subsurface* wlSubsurface_;
    };
};


}
