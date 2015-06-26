#pragma once

#include "waylandInclude.hpp"

#include "app/cursor.hpp"
#include "backends/windowContext.hpp"
#include "window/windowEvents.hpp"

#include <wayland-client-protocol.h>

#ifdef WithGL
#include <wayland-egl.h>
#include <EGL/egl.h>
#endif // WithGL

namespace ny
{

class waylandEventData : public eventData
{
public:
    waylandEventData(unsigned int xserial) : serial(xserial) {};
    unsigned int serial;
};

class waylandWindowContextSettings : public windowContextSettings{};

//wc///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//waylandWindowContext///////////////////////////////////////////////////////////////////////////////////////////////////
class waylandWindowContext : public virtual windowContext
{
protected:
    wl_surface* wlSurface_;
    wl_callback* wlFrameCallback_; //it this is == nullptr, the window is ready to be redrawn, else wayland is rendering the framebuffer and it should not be redrawn directly

    waylandAppContext* context_;
    bool refreshFlag_;

public:
    waylandWindowContext(window& win, const waylandWindowContextSettings& s = waylandWindowContextSettings());
    virtual ~waylandWindowContext();

    //high level functions//////////////////////////////////////////////////////////////////////////////////////////////
    virtual void refresh();

    virtual drawContext& beginDraw() = 0;
    virtual void finishDraw() = 0;

    virtual void show(){};
    virtual void hide(){};

    virtual void raise(){};
    virtual void lower(){};

    virtual void requestFocus(){};

    virtual void setWindowHints(unsigned long hints){};
    virtual void addWindowHints(unsigned long hint){};
    virtual void removeWindowHints(unsigned long hint){};

    virtual void setContextHints(unsigned long hints){};
    virtual void addContextHints(unsigned long hints){};
    virtual void removeContextHints(unsigned long hints){};

    virtual void setSize(vec2ui size, bool change = 1) = 0;
    virtual void setPosition(vec2i position, bool change = 1){};

    virtual void setCursor(const cursor& c);
    virtual void updateCursor(mouseCrossEvent* ev);

    virtual void sendContextEvent(contextEvent& e);

    //wayland specific functions//////////////////////////////////////////////////////////////////////////////////////////////
    wl_surface* getWlSurface() const { return wlSurface_; };
    waylandAppContext* getAppContext() const { return context_; };
};

//waylandToplevel////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class waylandToplevelWindowContext : public toplevelWindowContext, public waylandWindowContext
{
protected:
    wl_shell_surface* wlShellSurface_;

public:
    waylandToplevelWindowContext(toplevelWindow& win, const waylandWindowContextSettings& s = waylandWindowContextSettings());

    virtual void setMaximized();
    virtual void setMinimized();
    virtual void setFullscreen();
    virtual void setNormal();

    virtual void beginMove(mouseButtonEvent* ev);
    virtual void beginResize(mouseButtonEvent* ev, windowEdge edges);

    virtual void setBorderSize(unsigned int size){ };


    wl_shell_surface* getWlShellSurface() const { return wlShellSurface_; };
};

//waylandChild//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class waylandChildWindowContext : public childWindowContext, public waylandWindowContext
{
protected:
    wl_subsurface* wlSubsurface_;

public:
    waylandChildWindowContext(childWindow& win, const waylandWindowContextSettings& s = waylandWindowContextSettings());

    wl_subsurface* getWlSubsurface() const { return wlSubsurface_; };
};


}
