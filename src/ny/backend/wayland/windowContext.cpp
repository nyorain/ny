#include <ny/backend/wayland/windowContext.hpp>

#include <ny/event.hpp>
#include <ny/app.hpp>
#include <ny/error.hpp>
#include <ny/cursor.hpp>

#include <ny/wayland/xdg-shell-client-protocol.h>

#include <iostream>
#include <cassert>

namespace ny
{

waylandWindowContext::waylandWindowContext(window& win, const waylandWindowContextSettings& settings) : windowContext(win, settings)
{
    waylandAppContext* ac = getWaylandAppContext();
    if(!ac)
    {
        throw std::runtime_error("wayland App Context not corRectly initialized");
        return;
    }

    if(!ac->getWlCompositor())
    {
        throw std::runtime_error("wayland App Context has no compositor");
        return;
    }

    wlSurface_ = wl_compositor_create_surface(ac->getWlCompositor());
    if(!wlSurface_)
    {
        throw std::runtime_error("could not create wayland Surface");
        return;
    }

    wl_surface_set_user_data(wlSurface_, this);

    //window role
    auto* toplvlw = dynamic_cast<toplevelWindow*>(&win);
    auto* childw = dynamic_cast<childWindow*>(&win);
    if((!toplvlw && !childw) || (toplvlw && childw))
    {
        throw std::runtime_error("window must be either of childWindow or of toplevelWindow type");
        return;
    }

    if(toplvlw)
    {
        //if(ac->getXDGShell())
        //    createXDGSurface();
        //else
            createShellSurface();
    }
    else if(childw)
    {
        //if(ac->getXDGShell())
        //    createSubsurface();
        //else
            createSubsurface();
    }

    //drawContext
    #if (!defined NY_WithEGL && !defined NY_WithCairo)
    throw std::runtime_error("waylandWC::waylandWC: no renderer available");
    return;
    #endif

    bool gl = 0;

    #if (!defined NY_WithEGL)
    if(settings.glPref == preference::Must)
    {
        throw std::runtime_error("glPref set to must, but no gl renderer available");
        return;
    }

    #else
    gl = 1;

    #endif //NoCairo

    #if (!defined NY_WithCairo)
    if(settings.glPref == preference::MustNot)
    {
        throw std::runtime_error("glPref set to mustNot, but no software renderer available");
        return;
    }
    #else
    if(settings.glPref != preference::Must && settings.glPref != preference::Should)
        gl = 0;

    #endif //NoCairo

    if(gl)
    {
        drawType_ = waylandDrawType::egl;
        egl_ = new waylandEGLDrawContext(*this);
    }
    else
    {
        drawType_ = waylandDrawType::cairo;
        cairo_ = new waylandCairoDrawContext(*this);
    }

    nyMainApp()->sendEvent(refreshEvent(&getWindow()));
}

waylandWindowContext::~waylandWindowContext()
{
    if(wlFrameCallback_ != nullptr)
        wl_Callback_destroy(wlFrameCallback_); //needed? is it automatically destroy by the wayland server?

    //role
    if(getWlShellSurface())
    {
        wl_shell_surface_destroy(wlShellSurface_);
    }
    else if(getWlSubsurface())
    {
        wl_subsurface_destroy(wlSubsurface_);
    }

    else if(getXDGSurface())
    {
        xdg_surface_destroy(xdgSurface_);
    }
    else if(getXDGPopup())
    {
        xdg_popup_destroy(xdgPopup_);
    }

    //dc
    if(drawType_ == waylandDrawType::cairo && cairo_)
    {
        delete cairo_;
    }
    else if(drawType_ == waylandDrawType::egl && egl_)
    {
        delete egl_;
    }

    wl_surface_destroy(wlSurface_);
}

void waylandWindowContext::createShellSurface()
{
    waylandAppContext* ac = getWaylandAppContext();
    toplevelWindow* tw = dynamic_cast<toplevelWindow*>(&window_);
    if(!tw)
    {
        throw std::runtime_error("waylandWC::waylandWC: window has toplevel hint but is not a toplevelWindow");
        return;
    }


    if(!ac->getWlShell())
    {
        throw std::runtime_error("wayland App Context has no shell");
        return;
    }

    role_ = waylandSurfaceRole::shell;
    wlShellSurface_ = wl_shell_get_shell_surface(ac->getWlShell(), wlSurface_);

    if(!wlShellSurface_)
    {
        throw std::runtime_error("failed to create wl_shell_surface");
        return;
    }

    wl_shell_surface_set_toplevel(wlShellSurface_);
    wl_shell_surface_set_user_data(wlShellSurface_, this);

    wl_shell_surface_set_class(wlShellSurface_, nyMainApp()->getName().c_str());
    wl_shell_surface_set_title(wlShellSurface_, tw->getTitle().c_str());

    wl_shell_surface_add_listener(wlShellSurface_, &shellSurfaceListener, this);
}

void waylandWindowContext::createXDGSurface()
{
    waylandAppContext* ac = getWaylandAppContext();
    if(!ac->getXDGShell())
    {
        throw std::runtime_error("wayland App Context has no xdg_shell");
        return;
    }

    xdgSurface_ = xdg_shell_get_xdg_surface(ac->getXDGShell(), wlSurface_);
    if(xdgSurface_)
    {
        throw std::runtime_error("failed to create xdg_surface");
        return;
    }

    xdg_surface_set_window_geometry(xdgSurface_, window_.getPositionX(), window_.getPositionY(), window_.getWidth(), window_.getHeight());
    xdg_surface_set_user_data(xdgSurface_, this);

    xdg_surface_add_listener(xdgSurface_, &wayland::xdgSurfaceListener, this);
}

void waylandWindowContext::createSubsurface()
{
    waylandAppContext* ac = getWaylandAppContext();
    if(!ac->getWlSubcompositor())
    {
        throw std::runtime_error("wayland App Context has no shell");
        return;
    }

    childWindow* cw = dynamic_cast<childWindow*>(&window_);
    if(!cw)
    {
        throw std::runtime_error("waylandWC::waylandWC: window has child hint but is not a childWindow");
        return;
    }

    wl_surface* wlParent = nullptr;
    if(!asWayland(cw->getParent()->getWC()) || !(wlParent = asWayland(cw->getParent()->getWC())->getWlSurface()))
    {
        throw std::runtime_error("waylandWC::waylandWC: could not find wayland parent for child window");
        return;
    }

    role_ = waylandSurfaceRole::sub;
    wlSubsurface_ = wl_subcompositor_get_subsurface(ac->getWlSubcompositor(), wlSurface_, wlParent);

    if(!wlSubsurface_)
    {
        throw std::runtime_error("failed to create wl_subsurface");
        return;
    }

    wl_subsurface_set_position(wlSubsurface_, window_.getPositionX(), window_.getPositionY());
    wl_subsurface_set_user_data(wlSubsurface_, this);

    wl_subsurface_set_desync(wlSubsurface_);
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

    //redraw();
    getWindow().processEvent(drawEvent(&getWindow()));
}

drawContext* waylandWindowContext::beginDraw()
{
    if(getCairo() && cairo_)
    {
        if(cairo_->frontBufferUsed())
            cairo_->swapBuffers();

        cairo_->updateSize(getWindow().getSize());
        return cairo_;
    }
    else if(getEGL() && egl_)
    {
        egl_->initEGL(*this);

        if(!egl_->makeCurrent())
            return nullptr;

        egl_->updateViewport(getWindow().getSize());

        return egl_;
    }
    else
    {
        return nullptr;
    }
}

void waylandWindowContext::finishDraw()
{
    if(getCairo() && cairo_)
    {
        cairo_->apply();

        wlFrameCallback_ = wl_surface_frame(wlSurface_);
        wl_Callback_add_listener(wlFrameCallback_, &frameListener, this);

        cairo_->attach();
        wl_surface_damage(wlSurface_, 0, 0, window_.getWidth(), window_.getHeight());
        wl_surface_commit(wlSurface_);

        wl_display_flush(getWaylandAC()->getWlDisplay());
    }
    else if(getEGL() && egl_)
    {
        egl_->apply();

        wlFrameCallback_ = wl_surface_frame(wlSurface_);
        wl_Callback_add_listener(wlFrameCallback_, &frameListener, this);

        if(!egl_->swapBuffers())
            nyWarning("waylandWC::finishDraw: failed to swap egl buffers");

        if(!egl_->makeNotCurrent())
            nyWarning("waylandWC::finishDraw: failed to make egl context not current");
    }
    else
    {
        throw std::runtime_error("waylandWindowContext::finishDraw: uninitialized context");
    }
}

void waylandWindowContext::show()
{

}

void waylandWindowContext::hide()
{

}

void waylandWindowContext::addWindowHints(unsigned long hint)
{

}
void waylandWindowContext::removeWindowHints(unsigned long hint)
{

}

void waylandWindowContext::addContextHints(unsigned long hints)
{

}
void waylandWindowContext::removeContextHints(unsigned long hints)
{

}

void waylandWindowContext::setSize(Vec2ui size, bool change)
{
    if(getEGL())
    {
        egl_->setSize(size);
    }

    refresh();
}

void waylandWindowContext::setPosition(Vec2i position, bool change)
{
    if(getWlSubsurface())
    {
        if(change) wl_subsurface_set_position(wlSubsurface_, position.x, position.y);
    }
}

unsigned long waylandWindowContext::getAdditionalWindowHints() const
{
    unsigned long ret = 0;

    if(getWlShellSurface()) //toplevel
    {
        //ret |= windowHints::CustomDecorated | windowHints::CustomMoved | windowHints::CustomResized;
    }

    if(getEGL())
    {
        //ret |= windowHints::GL;
    }

    return ret;
}

void waylandWindowContext::setCursor(const cursor& c)
{
    //window class still stores the cursor, just update it
    updateCursor(nullptr);
}

void waylandWindowContext::updateCursor(const mouseCrossEvent* ev)
{
    unsigned int serial = 0;

    if(ev && ev->data)
    {
        waylandEventData* e = dynamic_cast<waylandEventData*>(ev->data.get());
        if(e) serial = e->serial;
    }

    if(window_.getCursor().isNativeType())
        getWaylandAC()->setCursor(cursorToWayland(window_.getCursor().getNativeType()), serial);

    else if(window_.getCursor().isImage())
        getWaylandAC()->setCursor(window_.getCursor().getImage(), window_.getCursor().getImageHotspot(), serial);
}

void waylandWindowContext::processEvent(const contextEvent& e)
{
    if(e.contextType() == frameEvent)
    {
        if(wlFrameCallback_)
        {
            wl_Callback_destroy(wlFrameCallback_); //?
            wlFrameCallback_ = nullptr;
        }

        if(refreshFlag_)
        {
            refreshFlag_ = 0;
            redraw();
        }
    }
}

void waylandWindowContext::setMaximized()
{
    if(getWlShellSurface()) wl_shell_surface_set_maximized(wlShellSurface_, nullptr);
}

void waylandWindowContext::setFullscreen()
{
    if(getWlShellSurface()) wl_shell_surface_set_fullscreen(wlShellSurface_, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 100, nullptr);
}

void waylandWindowContext::setMinimized()
{
    //??
}
void waylandWindowContext::setNormal()
{
    if(getWlShellSurface()) wl_shell_surface_set_toplevel(wlShellSurface_);
}
void waylandWindowContext::beginMove(const mouseButtonEvent* ev)
{
    waylandEventData* e = dynamic_cast<waylandEventData*> (ev->data.get());

    if(!e || !getWlShellSurface())
        return;

    wl_shell_surface_move(wlShellSurface_, getWaylandAC()->getWlSeat(), e->serial);
}

void waylandWindowContext::beginResize(const mouseButtonEvent* ev, windowEdge edge)
{
    waylandEventData* e = dynamic_cast<waylandEventData*> (ev->data.get());

    if(!e || !getWlShellSurface())
        return;

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

    wl_shell_surface_resize(wlShellSurface_, getWaylandAC()->getWlSeat(), e->serial, wlEdge);
}

void waylandWindowContext::setTitle(const std::string& str)
{

}

wl_shell_surface* WaylandWindowContext::wlShellSurface() const 
{ 
	return (role_ == SurfaceRole::shell) ? wlShellSurface_ : nullptr; 
}

wl_subsurface* WaylandWindowContext::wlSurbsurface() const 
{ 
	return (role_ == SurfaceRole::sub) ? wlSubsurface_ : nullptr; 
}

xdg_surface* WaylandWindowContext::xdgSurface() const 
{ 
	return (role_ == SurfaceRole::xdg) ? xdgSurface_ : nullptr; 
}

xdg_popup* WaylandWindowContext::xdgPopup() const 
{ 
	return (role_ == SurfaceRole::xdgPopup) ? xdgPopup_ : nullptr; 
}

}
