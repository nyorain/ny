#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/wayland/interfaces.hpp>
#include <ny/backend/wayland/util.hpp>
#include <ny/backend/wayland/xdg-shell-client-protocol.h>

#include <ny/base/event.hpp>
#include <ny/base/cursor.hpp>
#include <ny/base/log.hpp>
#include <ny/app/events.hpp>

#include <evg/drawContext.hpp>

#include <iostream>
#include <cassert>

namespace ny
{

WaylandWindowContext::WaylandWindowContext(WaylandAppContext& ac, 
	const WaylandWindowSettings& settings) : appContext_(&ac)
{
    wlSurface_ = wl_compositor_create_surface(&ac.wlCompositor());
    if(!wlSurface_) throw std::runtime_error("WaylandWC: could not create wl_surface");
    wl_surface_set_user_data(wlSurface_, this);

    //window role
	if(settings.parent.pointer())
	{
		auto& parent = *reinterpret_cast<wl_surface*>(settings.parent.pointer());
		createSubsurface(parent);
	}
	else
	{
		createShellSurface();
	}
}

WaylandWindowContext::~WaylandWindowContext()
{
    if(frameCallback_) wl_callback_destroy(frameCallback_);

    //role
    if(wlShellSurface()) wl_shell_surface_destroy(wlShellSurface_);
    else if(wlSubsurface()) wl_subsurface_destroy(wlSubsurface_);
    else if(xdgSurface()) xdg_surface_destroy(xdgSurface_);
    else if(xdgPopup()) xdg_popup_destroy(xdgPopup_);

    if(wlSurface_) wl_surface_destroy(wlSurface_);
}

void WaylandWindowContext::createShellSurface()
{
    if(!appContext_->wlShell()) throw std::runtime_error("WaylandWC: No wl_shell available");

    wlShellSurface_ = wl_shell_get_shell_surface(appContext_->wlShell(), wlSurface_);
    if(!wlShellSurface_) throw std::runtime_error("WaylandWC: failed to create wl_shell_surface");

    role_ = WaylandSurfaceRole::shell;

    wl_shell_surface_set_toplevel(wlShellSurface_);
    wl_shell_surface_set_user_data(wlShellSurface_, this);

    // wl_shell_surface_set_class(wlShellSurface_, nyMainApp()->getName().c_str());
    // wl_shell_surface_set_title(wlShellSurface_, tw->getTitle().c_str());

    wl_shell_surface_add_listener(wlShellSurface_, &wayland::shellSurfaceListener, this);
}

void WaylandWindowContext::createXDGSurface()
{
    if(!appContext_->xdgShell()) throw std::runtime_error("WaylandWC: no xdg_shell available");

    xdgSurface_ = xdg_shell_get_xdg_surface(appContext_->xdgShell(), wlSurface_);
    if(xdgSurface_) throw std::runtime_error("WaylandWC: failed to create xdg_surface");

    role_ = WaylandSurfaceRole::xdg;

    // xdg_surface_set_window_geometry(xdgSurface_, window_.getPositionX(), window_.getPositionY(), window_.getWidth(), window_.getHeight());
    xdg_surface_set_user_data(xdgSurface_, this);
    xdg_surface_add_listener(xdgSurface_, &wayland::xdgSurfaceListener, this);
}

void WaylandWindowContext::createSubsurface(wl_surface& parent)
{
	auto subcomp = appContext_->wlSubcompositor();
    if(!subcomp) throw std::runtime_error("WaylandWC: no wl_subcompositor");

    role_ = WaylandSurfaceRole::sub;
    wlSubsurface_ = wl_subcompositor_get_subsurface(subcomp, wlSurface_, &parent);

    if(!wlSubsurface_) throw std::runtime_error("WaylandWC: failed to create wl_subsurface");

    wl_subsurface_set_user_data(wlSubsurface_, this);
    wl_subsurface_set_desync(wlSubsurface_);
}

void WaylandWindowContext::refresh()
{
    if(frameCallback_)
    {
        refreshFlag_ = true;
        return;
    }

	if(eventHandler()) appContext_->dispatch(DrawEvent(eventHandler()));
}

DrawGuard WaylandWindowContext::draw()
{
	throw std::logic_error("WaylandWC::draw: drawless WindowContext");
    // if(getCairo() && cairo_)
    // {
    //     if(cairo_->frontBufferUsed())
    //         cairo_->swapBuffers();
// 
    //     cairo_->updateSize(getWindow().getSize());
    //     return cairo_;
    // }
    // else if(getEGL() && egl_)
    // {
    //     egl_->initEGL(*this);
// 
    //     if(!egl_->makeCurrent())
    //         return nullptr;
// 
    //     egl_->updateViewport(getWindow().getSize());
// 
    //     return egl_;
    // }
    // else
    // {
    //     return nullptr;
    // }
}

// void waylandWindowContext::finishDraw()
// {
//     if(getCairo() && cairo_)
//     {
//         cairo_->apply();
// 
//         wlFrameCallback_ = wl_surface_frame(wlSurface_);
//         wl_Callback_add_listener(wlFrameCallback_, &frameListener, this);
// 
//         cairo_->attach();
//         wl_surface_damage(wlSurface_, 0, 0, window_.getWidth(), window_.getHeight());
//         wl_surface_commit(wlSurface_);
// 
//         wl_display_flush(getWaylandAC()->getWlDisplay());
//     }
//     else if(getEGL() && egl_)
//     {
//         egl_->apply();
// 
//         wlFrameCallback_ = wl_surface_frame(wlSurface_);
//         wl_Callback_add_listener(wlFrameCallback_, &frameListener, this);
// 
//         if(!egl_->swapBuffers())
//             nyWarning("waylandWC::finishDraw: failed to swap egl buffers");
// 
//         if(!egl_->makeNotCurrent())
//             nyWarning("waylandWC::finishDraw: failed to make egl context not current");
//     }
//     else
//     {
//         throw std::runtime_error("waylandWindowContext::finishDraw: uninitialized context");
//     }
// }

void WaylandWindowContext::show()
{

}

void WaylandWindowContext::hide()
{

}

void WaylandWindowContext::addWindowHints(WindowHints hints)
{

}
void WaylandWindowContext::removeWindowHints(WindowHints hints)
{

}

void WaylandWindowContext::size(const Vec2ui& size)
{
}

void WaylandWindowContext::position(const Vec2i& position)
{
    if(wlSubsurface())
    {
       wl_subsurface_set_position(wlSubsurface_, position.x, position.y);
    }
	else
	{
		warning("WaylandWC::position: wayland does not support custom positionts");
	}
}

void WaylandWindowContext::cursor(const Cursor& c)
{
	//TODO
    //window class still stores the cursor, just update it
    // updateCursor(nullptr);
}

// void WaylandWindowContext::updateCursor(const mouseCrossEvent* ev)
// {
//     unsigned int serial = 0;
// 
//     if(ev && ev->data)
//     {
//         waylandEventData* e = dynamic_cast<waylandEventData*>(ev->data.get());
//         if(e) serial = e->serial;
//     }
// 
//     if(window_.getCursor().isNativeType())
//         getWaylandAC()->setCursor(cursorToWayland(window_.getCursor().getNativeType()), serial);
// 
//     else if(window_.getCursor().isImage())
//         getWaylandAC()->setCursor(window_.getCursor().getImage(), window_.getCursor().getImageHotspot(), serial);
// }

bool WaylandWindowContext::handleEvent(const Event& event)
{
    if(event.type() == eventType::wayland::frameEvent)
    {
        if(frameCallback_)
        {
            wl_callback_destroy(frameCallback_); //?
            frameCallback_ = nullptr;
        }

        if(refreshFlag_)
        {
            refreshFlag_ = 0;
			if(eventHandler()) appContext_->dispatch(DrawEvent(eventHandler()));
        }
    }
}

void WaylandWindowContext::maximize()
{
    if(wlShellSurface()) wl_shell_surface_set_maximized(wlShellSurface_, nullptr);
}

void WaylandWindowContext::fullscreen()
{
	// TODO: output param?
    if(wlShellSurface()) 
	{
		wl_shell_surface_set_fullscreen(wlShellSurface_, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 
			0, nullptr);
	}
	else if(xdgSurface())
	{
		xdg_surface_set_fullscreen(xdgSurface_, nullptr);
	}
}

void WaylandWindowContext::minimize()
{
	if(xdgSurface()) xdg_surface_set_minimized(xdgSurface_);
}
void WaylandWindowContext::normalState()
{
    if(wlShellSurface())
	{
		wl_shell_surface_set_toplevel(wlShellSurface_);
	}
	else if(xdgSurface())
	{
		//TODO
	}
}
void WaylandWindowContext::beginMove(const MouseButtonEvent* ev)
{
    auto* data = dynamic_cast<WaylandEventData*>(ev->data.get());
    if(!data || !appContext_->wlSeat()) return;

	if(wlShellSurface())
	{
		wl_shell_surface_move(wlShellSurface_, appContext_->wlSeat(), data->serial);
	}
	else if(xdgSurface())
	{
		xdg_surface_move(xdgSurface_, appContext_->wlSeat(), data->serial);
	}
}

void WaylandWindowContext::beginResize(const MouseButtonEvent* ev, WindowEdges edge)
{
    auto* data = dynamic_cast<WaylandEventData*>(ev->data.get());

    if(!data || !appContext_->wlSeat()) return;

    // unsigned int wlEdge = static_cast<unsigned int>(edge);
    unsigned int wlEdge = edge.value();
    wl_shell_surface_resize(wlShellSurface_, appContext_->wlSeat(), data->serial, wlEdge);
}

void WaylandWindowContext::title(const std::string& titlestring)
{
	if(wlShellSurface_) wl_shell_surface_set_title(wlShellSurface_, titlestring.c_str());
	else if(xdgSurface_) xdg_surface_set_title(xdgSurface_, titlestring.c_str());
}

wl_shell_surface* WaylandWindowContext::wlShellSurface() const 
{ 
	return (role_ == WaylandSurfaceRole::shell) ? wlShellSurface_ : nullptr; 
}

wl_subsurface* WaylandWindowContext::wlSubsurface() const 
{ 
	return (role_ == WaylandSurfaceRole::sub) ? wlSubsurface_ : nullptr; 
}

xdg_surface* WaylandWindowContext::xdgSurface() const 
{ 
	return (role_ == WaylandSurfaceRole::xdg) ? xdgSurface_ : nullptr; 
}

xdg_popup* WaylandWindowContext::xdgPopup() const 
{ 
	return (role_ == WaylandSurfaceRole::xdgPopup) ? xdgPopup_ : nullptr; 
}

}


    // case WindowEdge::top:
    //     wlEdge = WL_SHELL_SURFACE_RESIZE_TOP;
    //     break;
    // case WindowEdge::left:
    //     wlEdge = WL_SHELL_SURFACE_RESIZE_LEFT;
    //     break;
    // case WindowEdge::bottom:
    //     wlEdge = WL_SHELL_SURFACE_RESIZE_BOTTOM;
    //     break;
    // case WindowEdge::right:
    //     wlEdge = WL_SHELL_SURFACE_RESIZE_RIGHT;
    //     break;
    // case WindowEdge::topLeft:
    //     wlEdge = WL_SHELL_SURFACE_RESIZE_TOP_LEFT;
    //     break;
    // case WindowEdge::topRight:
    //     wlEdge = WL_SHELL_SURFACE_RESIZE_TOP_RIGHT;
    //     break;
    // case WindowEdge::bottomLeft:
    //     wlEdge = WL_SHELL_SURFACE_RESIZE_BOTTOM_LEFT;
    //     break;
    // case WindowEdge::bottomRight:
    //     wlEdge = WL_SHELL_SURFACE_RESIZE_BOTTOM_RIGHT;
    //     break;
    // default:
    //     return;
