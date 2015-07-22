#include <ny/wayland/waylandWindowContext.hpp>

#include <ny/wayland/waylandUtil.hpp>
#include <ny/wayland/waylandAppContext.hpp>
#include <ny/wayland/waylandInterfaces.hpp>

#include <ny/event.hpp>
#include <ny/app.hpp>
#include <ny/error.hpp>
#include <ny/cursor.hpp>

#include <iostream>

#ifdef NY_WithEGL
#include <ny/gl/glDrawContext.hpp>
#include <ny/wayland/waylandEgl.hpp>
#endif // NY_WithEGL

#ifdef NY_WithCairo
#include <ny/cairo.hpp>
#include <ny/wayland/waylandCairo.hpp>
#endif // NY_WithCairo

namespace ny
{

using namespace wayland;

//waylandWindowContext/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
waylandWindowContext::waylandWindowContext(window& win, const waylandWindowContextSettings& settings) : windowContext(win, settings)
{
    waylandAppContext* ac = getWaylandAppContext();
    if(!ac)
    {
        throw std::runtime_error("wayland App Context not correctly initialized");
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

    unsigned long hints = win.getWindowHints();

    //window role
    if(hints & windowHints::Toplevel)
    {
        toplevelWindow* tw = dynamic_cast<toplevelWindow*>(&win);
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

        wl_shell_surface_set_class(wlShellSurface_, tw->getName().c_str());
        wl_shell_surface_set_title(wlShellSurface_, tw->getName().c_str());

        wl_shell_surface_add_listener(wlShellSurface_, &shellSurfaceListener, this);
    }
    else if(hints & windowHints::Child)
    {
        if(!ac->getWlSubcompositor())
        {
            throw std::runtime_error("wayland App Context has no shell");
            return;
        }

        childWindow* cw = dynamic_cast<childWindow*>(&win);
        if(!cw)
        {
            throw std::runtime_error("waylandWC::waylandWC: window has child hint but is not a childWindow");
            return;
        }

        wl_surface* wlParent = asWayland(cw->getParent()->getWC())->getWlSurface();
        if(!wlParent)
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

        wl_subsurface_set_position(wlSubsurface_, win.getPositionX(), win.getPositionY());
        wl_subsurface_set_user_data(wlSubsurface_, this);

        wl_subsurface_set_desync(wlSubsurface_);
    }
    else
    {
        throw std::runtime_error("window has no type defined in its hints");
        return;
    }

    //drawContext
    #if (defined NY_WithCairo && defined NY_WithEGL)
    if(settings.glPref == preference::MustNot || settings.glPref == preference::ShouldNot)
    {
        drawType_ = waylandDrawType::cairo;
        cairo_ = new waylandCairoDrawContext(*this);
    }
    else
    {
        drawType_ = waylandDrawType::egl;
        egl_ = new waylandEGLDrawContext(*this);
    }

    //Cairo, but no EGL
    #elif (defined NY_WithCairo && !defined NY_WithEGL)
    if(settings.glPref == preference::Must)
    {
        throw std::runtime_error("waylandWC::waylandWC: no gl renderer available, glpref was set to <must>");
        return;
    }
    else
    {
        drawType_ = waylandDrawType::cairo;
        cairo_ = new waylandCairoContext(*this);
    }

    //EGL, but no cairo
    #elif (!defined NY_WithCairo && defined NY_WithEGL)
    if(settings.glPref == preference::MustNot)
    {
        throw std::runtime_error("waylandWC::waylandWC: no software renderer available, glpref was set to <mustNot>");
        return;
    }
    else
    {
        drawType_ = waylandDrawType::egl;
        egl_ = new waylandEGLContext(*this);
    }

    //neither egl nor cairo
    #else
    throw std::runtime_error("waylandWC::waylandWC: no available renderer");
    return;

    #endif //renderer
}

waylandWindowContext::~waylandWindowContext()
{
    if(wlFrameCallback_ != nullptr)
        wl_callback_destroy(wlFrameCallback_); //needed? is it automatically destroy by the wayland server?

    if(role_ == waylandSurfaceRole::shell && wlShellSurface_)
    {
        wl_shell_surface_destroy(wlShellSurface_);
    }
    else if(role_ == waylandSurfaceRole::sub && wlSubsurface_)
    {
        wl_subsurface_destroy(wlSubsurface_);
    }

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

drawContext& waylandWindowContext::beginDraw()
{
    if(getCairo() && cairo_)
    {
        return *cairo_;
    }
    else if(getEGL() && egl_)
    {
        return *egl_;
    }
    else
    {
        throw std::runtime_error("waylandWindowContext::beginDraw: uninitialized context");
    }
}

void waylandWindowContext::finishDraw()
{
    if(getCairo() && cairo_)
    {
        cairo_->apply();

        wlFrameCallback_ = wl_surface_frame(wlSurface_);
        wl_callback_add_listener(wlFrameCallback_, &frameListener, this);

        wl_surface_attach(wlSurface_, cairo_->getShmBuffer().getWlBuffer(), 0, 0);
        wl_surface_damage(wlSurface_, 0, 0, window_.getWidth(), window_.getHeight());
        wl_surface_commit(wlSurface_);
    }
    else if(getEGL() && egl_)
    {
        egl_->apply();

        wlFrameCallback_ = wl_surface_frame(wlSurface_);
        wl_callback_add_listener(wlFrameCallback_, &frameListener, this);
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

void waylandWindowContext::setSize(vec2ui size, bool change)
{

}

void waylandWindowContext::setPosition(vec2i position, bool change)
{

}

unsigned long waylandWindowContext::getAdditionalWindowHints() const
{
    return 0;
}

void waylandWindowContext::setCursor(const cursor& c)
{
    // - nothing to do here probably
    updateCursor(nullptr);
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
        getWaylandAC()->setCursor(cursorToWayland(window_.getCursor().getNativeType()), serial);

    else if(window_.getCursor().isImage())
        getWaylandAC()->setCursor(window_.getCursor().getImage(), window_.getCursor().getImageHotspot(), serial);
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
void waylandWindowContext::beginMove(mouseButtonEvent* ev)
{
    waylandEventData* e = dynamic_cast<waylandEventData*> (ev->data);

    if(!ev || !getWlShellSurface())
        return;

    wl_shell_surface_move(wlShellSurface_, getWaylandAC()->getWlSeat(), e->serial);
}

void waylandWindowContext::beginResize(mouseButtonEvent* ev, windowEdge edge)
{
    waylandEventData* e = dynamic_cast<waylandEventData*> (ev->data);

    if(!ev || !getWlShellSurface())
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

void waylandWindowContext::setName(std::string str)
{

}

}
