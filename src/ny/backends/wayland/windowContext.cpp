#include "backends/wayland/windowContext.hpp"

#include "backends/wayland/utils.hpp"
#include "backends/wayland/defs.hpp"
#include "backends/wayland/appContext.hpp"

#include "app/event.hpp"
#include "app/app.hpp"
#include "app/error.hpp"
#include "app/cursor.hpp"

#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <linux/input.h>

namespace ny
{

using namespace wayland;

//waylandWindowContext/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
waylandWindowContext::waylandWindowContext(window& win, const waylandWindowContextSettings& settings) : windowContext(win, settings), wlSurface_(nullptr), wlFrameCallback_(nullptr), context_(nullptr)
{
    context_ = asWayland(getMainApp()->getAppContext());
    if(!context_)
    {
        throw error(error::Critical, "wayland App Context not correctly initialized");
        return;
    }

    wl_compositor* compositor = context_->getWlCompositor();
    if(!compositor)
    {
        throw error(error::Critical, "wayland App Context not correctly initialized");
        return;
    }

    wlSurface_ = wl_compositor_create_surface(compositor);
    if(!wlSurface_)
    {
        throw error(error::Critical, "could not create wayland Surface");
        return;
    }

    wl_surface_set_user_data(wlSurface_, this);
}

waylandWindowContext::~waylandWindowContext()
{
    wl_surface_destroy(wlSurface_);

    if(wlFrameCallback_ != nullptr)
    {
        //needed? is it automatically destroy by the wayland server?
        wl_callback_destroy(wlFrameCallback_);
    }
}

void waylandWindowContext::refresh()
{
    if(wlSurface_ == nullptr)
        return;

    if(wlFrameCallback_ != nullptr)
    {
        refreshFlag_ = 1;
        return;
    }

    redraw();
}

void waylandWindowContext::setCursor(const cursor& c)
{
}

void waylandWindowContext::updateCursor(mouseCrossEvent* ev)
{
    unsigned int serial = 0;

    if(ev && ev->data)
    {
        waylandEventData* e = dynamic_cast<waylandEventData*>(ev->data);
        if(e) serial = e->serial;
    }

    if(window_.getCursor().isNativeType())
        context_->setCursor(cursorToWayland(window_.getCursor().getNativeType()), serial);

        //todo::image
}

void waylandWindowContext::sendContextEvent(contextEvent& e)
{
    if(e.contextEventType == frameEvent)
    {
        wl_callback_destroy(wlFrameCallback_);
        wlFrameCallback_ = nullptr;

        if(refreshFlag_)
        {
            refreshFlag_ = 0;
            redraw();
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////7
waylandToplevelWindowContext::waylandToplevelWindowContext(toplevelWindow& win, const waylandWindowContextSettings& settings) : windowContext(win, settings), toplevelWindowContext(win, settings), waylandWindowContext(win, settings), wlShellSurface_(nullptr)
{
    wlShellSurface_ = wl_shell_get_shell_surface(context_->getWlShell(), wlSurface_);
    wl_shell_surface_set_toplevel(wlShellSurface_);
    wl_shell_surface_set_title(wlShellSurface_, win.getName().c_str());
    wl_shell_surface_set_class(wlShellSurface_, win.getName().c_str());
    wl_shell_surface_set_user_data(wlShellSurface_, this);
    wl_shell_surface_add_listener(wlShellSurface_, &shellSurfaceListener, this);
}

void waylandToplevelWindowContext::setMaximized()
{
    wl_shell_surface_set_maximized(wlShellSurface_, nullptr);
}

void waylandToplevelWindowContext::setFullscreen()
{
    wl_shell_surface_set_fullscreen(wlShellSurface_, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 100, nullptr);
}

void waylandToplevelWindowContext::setMinimized()
{
    //??
}

void waylandToplevelWindowContext::setNormal()
{
    wl_shell_surface_set_toplevel(wlShellSurface_);
}

void waylandToplevelWindowContext::beginMove(mouseButtonEvent* ev)
{
    waylandEventData* e = dynamic_cast<waylandEventData*> (ev->data);

    if(!ev || !wlShellSurface_)
    {
        return;
    }

    wl_shell_surface_move(wlShellSurface_, context_->getWlSeat(), e->serial);
}

void waylandToplevelWindowContext::beginResize(mouseButtonEvent* ev, windowEdge edge)
{
    waylandEventData* e = dynamic_cast<waylandEventData*> (ev->data);

    if(!ev || !wlShellSurface_)
    {
        return;
    }

    unsigned int wlEdge = 0;

    switch(edge)
    {
    case windowEdge::Top:
        wlEdge = WL_SHELL_SURFACE_RESIZE_TOP;
        break;
    case windowEdge::Left:
        wlEdge = WL_SHELL_SURFACE_RESIZE_LEFT;
        break;
    case windowEdge::Bottom:
        wlEdge = WL_SHELL_SURFACE_RESIZE_BOTTOM;
        break;
    case windowEdge::Right:
        wlEdge = WL_SHELL_SURFACE_RESIZE_RIGHT;
        break;
    case windowEdge::TopLeft:
        wlEdge = WL_SHELL_SURFACE_RESIZE_TOP_LEFT;
        break;
    case windowEdge::TopRight:
        wlEdge = WL_SHELL_SURFACE_RESIZE_TOP_RIGHT;
        break;
    case windowEdge::BottomLeft:
        wlEdge = WL_SHELL_SURFACE_RESIZE_BOTTOM_LEFT;
        break;
    case windowEdge::BottomRight:
        wlEdge = WL_SHELL_SURFACE_RESIZE_BOTTOM_RIGHT;
        break;
    default:
        return;
    }

    wl_shell_surface_resize(wlShellSurface_, context_->getWlSeat(), e->serial, wlEdge);
}



////////////////////////////////////////////////////////////////////////////////////
waylandChildWindowContext::waylandChildWindowContext(childWindow& win, const waylandWindowContextSettings& settings) : windowContext(win, settings), childWindowContext(win, settings), waylandWindowContext(win, settings), wlSubsurface_(nullptr)
{
    window* parent = win.getParent();

    waylandWC* parentWC = asWayland(parent->getWindowContext());
    wlSubsurface_ = wl_subcompositor_get_subsurface(context_->getWlSubcompositor(), wlSurface_, parentWC->getWlSurface());
    wl_subsurface_set_position(wlSubsurface_, win.getPosition().x, win.getPosition().y);
    wl_subsurface_set_desync(wlSubsurface_);

    wl_subsurface_set_user_data(wlSubsurface_, this);
}


}
