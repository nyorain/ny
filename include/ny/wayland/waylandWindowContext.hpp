#pragma once

#include <ny/wayland/waylandInclude.hpp>

#include <ny/cursor.hpp>
#include <ny/windowContext.hpp>
#include <ny/windowEvents.hpp>

#include <wayland-client-protocol.h>

#ifdef NY_WithGL
#include <wayland-egl.h>
#include <EGL/egl.h>
#endif // NY_WithGL


struct xdg_surface;
struct xdg_popup;

namespace ny
{

class waylandEventData : public eventDataBase<waylandEventData>
{
public:
    waylandEventData(unsigned int xserial) : serial(xserial) {};
    unsigned int serial;
};

class waylandWindowContextSettings : public windowContextSettings{};

enum class waylandSurfaceRole : unsigned char
{
    none,

    shell,
    sub,
    xdg,
    xdgPopup
};

enum class waylandDrawType : unsigned char
{
    none,

    cairo,
    egl
};

//wc////////////////////////////////////////////////////////////
//waylandWindowContext//////////////////////////////////////////////////
class waylandWindowContext : public windowContext
{
private:
    //util functions
    void createShellSurface();
    void createXDGSurface();
    void createXDGPopup();
    void createSubsurface();

protected:
    wl_surface* wlSurface_ = nullptr;
    wl_callback* wlFrameCallback_ = nullptr; //if this is == nullptr, the window is ready to be redrawn, else wayland is rendering the framebuffer and it should not be redrawn directly

    bool refreshFlag_ = 0; //signals, if window should be refreshed

    //role
    waylandSurfaceRole role_ = waylandSurfaceRole::none;
    union
    {
        wl_shell_surface* wlShellSurface_ = nullptr;
        xdg_surface* xdgSurface_;
        xdg_popup* xdgPopup_;
        wl_subsurface* wlSubsurface_;
    };

    //draw
    waylandDrawType drawType_ = waylandDrawType::none;
    union
    {
        waylandEGLDrawContext* egl_ = nullptr;
        waylandCairoDrawContext* cairo_;
    };

public:
    waylandWindowContext(window& win, const waylandWindowContextSettings& s = waylandWindowContextSettings());
    virtual ~waylandWindowContext();

    //high level functions///////////////////////////////////////////////
    virtual void refresh() override;

    virtual drawContext& beginDraw() override;
    virtual void finishDraw() override;

    virtual void show() override;
    virtual void hide() override;

    virtual void addWindowHints(unsigned long hint) override;
    virtual void removeWindowHints(unsigned long hint) override;

    virtual void addContextHints(unsigned long hints) override;
    virtual void removeContextHints(unsigned long hints) override;

    virtual void setSize(vec2ui size, bool change = 1) override;
    virtual void setPosition(vec2i position, bool change = 1) override;

    virtual void setCursor(const cursor& c) override;
    virtual void updateCursor(mouseCrossEvent* ev) override;

    virtual void sendContextEvent(std::unique_ptr<contextEvent> e) override;

    virtual unsigned long getAdditionalWindowHints() const override;

    virtual bool hasGL() const override { return (drawType_ == waylandDrawType::egl); }

    //toplevel
    virtual void setMaximized() override;
    virtual void setMinimized() override;
    virtual void setFullscreen() override;
    virtual void setNormal() override;

    virtual void beginMove(mouseButtonEvent* ev) override;
    virtual void beginResize(mouseButtonEvent* ev, ny::windowEdge edge) override;

    virtual void setTitle(const std::string& str) override;

    //wayland specific functions///////////////////////////////////////////////
    wl_surface* getWlSurface() const { return wlSurface_; };

    waylandDrawType getDrawType() const { return drawType_; }
    waylandSurfaceRole getSurfaceRole() const { return role_; }

    wl_shell_surface* getWlShellSurface() const { return (role_ == waylandSurfaceRole::shell) ? wlShellSurface_ : nullptr; }
    wl_subsurface* getWlSubsurface() const { return (role_ == waylandSurfaceRole::sub) ? wlSubsurface_ : nullptr; }
    xdg_surface* getXDGSurface() const { return (role_ == waylandSurfaceRole::xdg) ? xdgSurface_ : nullptr; }
    xdg_popup* getXDGPopup() const { return (role_ == waylandSurfaceRole::xdgPopup) ? xdgPopup_ : nullptr; }

    waylandCairoDrawContext* getCairo() const { return (drawType_ == waylandDrawType::cairo) ? cairo_ : nullptr; }
    waylandEGLDrawContext* getEGL() const { return (drawType_ == waylandDrawType::egl) ? egl_ : nullptr; }
};


}
