#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/wayland/util.hpp> //for wayland::ShmBuffer. Problem?

#include <ny/backend/windowContext.hpp>
#include <ny/backend/windowSettings.hpp>
#include <nytl/vec.hpp>

namespace ny
{

///Specifies the different roles a WaylandWindowContext can have.
enum class WaylandSurfaceRole : unsigned int
{
    none,
    shell,
    sub,
    xdg,
    xdgPopup
};

///WindowSettings class for wayland WindowContexts.
class WaylandWindowSettings : public WindowSettings {};

///WaylandDrawIntegration
class WaylandDrawIntegration
{
public:
	WaylandDrawIntegration(WaylandWindowContext& wc);
	virtual ~WaylandDrawIntegration();
	virtual void resize(const nytl::Vec2ui&) {}

protected:
	WaylandWindowContext& windowContext_;
};


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

    void droppable(const DataTypes&) override;

    void minSize(const nytl::Vec2ui&) override;
    void maxSize(const nytl::Vec2ui&) override;

    void size(const nytl::Vec2ui& size) override;
    void position(const nytl::Vec2i& position) override;

    void cursor(const Cursor& c) override;
	NativeWindowHandle nativeHandle() const override;

	WindowCapabilities capabilities() const override;

    //toplevel
    void maximize() override;
    void minimize() override;
    void fullscreen() override;
    void normalState() override;

    void beginMove(const MouseButtonEvent* ev) override;
    void beginResize(const MouseButtonEvent* event, WindowEdges edges) override;

    void title(const std::string& name) override;
	void icon(const ImageData&) override {}

	bool customDecorated() const override { return true; }
	void addWindowHints(WindowHints hints) override;
	void removeWindowHints(WindowHints hints) override;

	//
	bool handleEvent(const Event& event) override;

    //wayland specific functions
    wl_surface& wlSurface() const { return *wlSurface_; };
	wl_callback* frameCallback() const { return frameCallback_; }
	WaylandSurfaceRole surfaceRole() const { return role_; }
	nytl::Vec2ui size() const { return size_; }
	bool shown() const { return shown_; }

    wl_shell_surface* wlShellSurface() const; 
    wl_subsurface* wlSubsurface() const; 
    xdg_surface* xdgSurface() const; 
    xdg_popup* xdgPopup() const; 

	wl_buffer* wlCursorBuffer() const { return cursorBuffer_; }
	nytl::Vec2i cursorHotspot() const { return cursorHotspot_; }
	nytl::Vec2ui cursorSize() const { return cursorSize_; }

	///Attaches the given buffer, damages the surface and commits it.
	///Does also add a frameCallback to the surface.
	///If called with a nullptr, no framecallback will be attached and the surface will
	///be unmapped. Note that if the WindowContext is hidden, no buffer will be 
	///attached.
	void attachCommit(wl_buffer* buffer);

	///Sets the integration to the given one.
	///Will return false if there is already such an integration or this implementation
	///does not support them (e.g. vulkan/opengl WindowContext).
	///Calling this function with a nullptr resets the integration.
	virtual bool drawIntegration(WaylandDrawIntegration* integration);

	///Creates a surface and stores it in the given parameter.
	///Returns false and does not change the given parameter if a surface coult not be
	///created.
	///This could be the case if the WindowContext already has another integration.
	virtual bool surface(Surface& surface);

	WaylandAppContext& appContext() const { return *appContext_; }
	wl_display& wlDisplay() const;

protected:
    //util functions
    void createShellSurface();
    void createXDGSurface();
    void createXDGPopup();
    void createSubsurface(wl_surface& parent);

protected:
	WaylandAppContext* appContext_ {};
    wl_surface* wlSurface_ {};
	nytl::Vec2ui size_ {};
	
	//if this is == nullptr, the window is ready to be redrawn.
	//otherwise waiting for the callback to be called
    wl_callback* frameCallback_ {};

	//stores if the window has a pending refresh request, i.e. if it should refresh
	//as soon as possible
	//flag will be set by refresh() and trigger a DrawEvent when the frameCallback is called
    bool refreshFlag_ = false;

    //stores which kinds of surface this context holds
	WaylandSurfaceRole role_ = WaylandSurfaceRole::none;

	//the different surface roles this surface can have.
	//the union will be activated depending on role_
	//xdg roles are preferred over the plain wl ones if available
    union
    {
        wl_shell_surface* wlShellSurface_ = nullptr;
        xdg_surface* xdgSurface_;
        xdg_popup* xdgPopup_;
        wl_subsurface* wlSubsurface_;
    };

	WaylandDrawIntegration* drawIntegration_ = nullptr; //optional assocated DrawIntegration
	bool shown_ {}; //Whether the WindowContext should be shown or hidden

	wayland::ShmBuffer shmCursorBuffer_ {}; //only needed when cursor is custom image

	wl_buffer* cursorBuffer_ {};
	nytl::Vec2i cursorHotspot_ {};
	nytl::Vec2ui cursorSize_ {};
};


}
