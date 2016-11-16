// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/interfaces.hpp>
#include <ny/wayland/input.hpp>
#include <ny/wayland/util.hpp>
#include <ny/wayland/xdg-shell-client-protocol.h>

#include <ny/common/unix.hpp>
#include <ny/mouseContext.hpp>
#include <ny/event.hpp>
#include <ny/cursor.hpp>
#include <ny/log.hpp>
#include <ny/events.hpp>

#include <wayland-cursor.h>

#include <iostream>
#include <cstring>

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
		createSubsurface(parent, settings);
	}
	else if(ac.xdgShell())
	{
		createXDGSurface(settings);
	}
	else
	{
		createShellSurface(settings);
	}

	size_ = settings.size;
	shown_ = settings.show;
	cursor(settings.cursor);
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

void WaylandWindowContext::createShellSurface(const WaylandWindowSettings& ws)
{
    if(!appContext_->wlShell()) throw std::runtime_error("WaylandWC: No wl_shell available");

    wlShellSurface_ = wl_shell_get_shell_surface(appContext_->wlShell(), wlSurface_);
    if(!wlShellSurface_) throw std::runtime_error("WaylandWC: failed to create wl_shell_surface");

    role_ = WaylandSurfaceRole::shell;

    wl_shell_surface_set_toplevel(wlShellSurface_);
    wl_shell_surface_set_user_data(wlShellSurface_, this);

    wl_shell_surface_add_listener(wlShellSurface_, &wayland::shellSurfaceListener, this);

	wl_shell_surface_set_title(wlShellSurface_, ws.title.c_str());
	//TODO: class (AppContextSettings)
}

void WaylandWindowContext::createXDGSurface(const WaylandWindowSettings& ws)
{
    if(!appContext_->xdgShell()) throw std::runtime_error("WaylandWC: no xdg_shell available");

    xdgSurface_ = xdg_shell_get_xdg_surface(appContext_->xdgShell(), wlSurface_);
    if(xdgSurface_) throw std::runtime_error("WaylandWC: failed to create xdg_surface");

    role_ = WaylandSurfaceRole::xdg;

	xdg_surface_set_window_geometry(xdgSurface_, 0, 0, size_.x, size_.y);
    xdg_surface_set_user_data(xdgSurface_, this);
    xdg_surface_add_listener(xdgSurface_, &wayland::xdgSurfaceListener, this);

	xdg_surface_set_title(xdgSurface_, ws.title.c_str());
	//TODO: app id (AppContextSettings)
}

void WaylandWindowContext::createSubsurface(wl_surface& parent, const WaylandWindowSettings&)
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

void WaylandWindowContext::show()
{
	shown_ = true;
}

void WaylandWindowContext::hide()
{
	shown_ = false;
	attachCommit(nullptr);
}

void WaylandWindowContext::addWindowHints(WindowHints hints)
{
	nytl::unused(hints);
}
void WaylandWindowContext::removeWindowHints(WindowHints hints)
{
	nytl::unused(hints);
}

void WaylandWindowContext::size(const Vec2ui& size)
{
	size_ = size;
}

void WaylandWindowContext::position(const Vec2i& position)
{
    if(wlSubsurface())
    {
		wl_subsurface_set_position(wlSubsurface_, position.x, position.y);
    }
	else
	{
		warning("ny::WaylandWC::position: wayland does not support custom positions");
	}
}

void WaylandWindowContext::cursor(const Cursor& cursor)
{
	if(cursor.type() == Cursor::Type::image)
	{
		auto* img = cursor.image();
		if(!img || !img->data)
		{
			warning("ny::WlWC::cursor: invalid image cursor");
			return;
		}

		shmCursorBuffer_ = wayland::ShmBuffer(appContext(), img->size, img->stride);
		if(img->format != ImageDataFormat::argb8888)
		{
			convertFormat(*img, ImageDataFormat::argb8888, shmCursorBuffer_.data());
		}
		else
		{
			std::memcpy(&shmCursorBuffer_.data(), img->data, shmCursorBuffer_.dataSize());
		}


		cursorHotspot_ = cursor.imageHotspot();
		cursorSize_ = img->size;
		cursorBuffer_ = &shmCursorBuffer_.wlBuffer();
	}
	else if(cursor.type() == Cursor::Type::none)
	{
		cursorHotspot_ = {};
		cursorSize_ = {};
		cursorBuffer_ = {};
	}
	else
	{
		auto cursorName = cursorToXName(cursor.type());
		auto cursorTheme = appContext().wlCursorTheme();
		auto* wlCursor = wl_cursor_theme_get_cursor(cursorTheme, cursorName);
		if(!wlCursor)
		{
			warning("ny::WlWC::cursor: failed to retrieve cursor ", cursorName);
			return;
		}

		//TODO: handle mulitple/animated images
		auto img = wlCursor->images[0];

		cursorBuffer_ = wl_cursor_image_get_buffer(img);

		cursorHotspot_.x = img->hotspot_y;
		cursorHotspot_.x = img->hotspot_x;

		cursorSize_.x = img->width;
		cursorSize_.y = img->height;
	}

	//ugly hack... TODO
	if(!appContext_->mouseContext()) return;

	//update the cursor if needed
	auto& wmc = appContext_->waylandMouseContext();
	if(wmc.over() == this)
		wmc.cursorBuffer(cursorBuffer_, cursorHotspot_, cursorSize_);
}

void WaylandWindowContext::droppable(const DataTypes&)
{
	//TODO
	//currently all windows are droppabe, store it here and check it in wayland/data.cpp
}

void WaylandWindowContext::minSize(const Vec2ui&)
{
	warning("WaylandWC::maxSize: wayland has no capability for size limits");
}
void WaylandWindowContext::maxSize(const Vec2ui&)
{
	warning("WaylandWC::maxSize: wayland has no capability for size limits");
}

NativeHandle WaylandWindowContext::nativeHandle() const
{
	return {wlSurface_};
}

WindowCapabilities WaylandWindowContext::capabilities() const
{
	//change this when using new xdg shell version
	//make it dependent on the actual shell used
	return WindowCapability::size |
		WindowCapability::fullscreen |
		WindowCapability::minimize |
		WindowCapability::maximize;
}

void WaylandWindowContext::configureEvent(nytl::Vec2ui size, WindowEdges)
{
	size_ = size;

	if(!eventHandler()) return;

	auto sizeEvent = SizeEvent(eventHandler());
	sizeEvent.size = size_;
	appContext().dispatch(std::move(sizeEvent));

	refresh();
}

void WaylandWindowContext::frameEvent()
{
	if(frameCallback_)
	{
		wl_callback_destroy(frameCallback_);
		frameCallback_ = nullptr;
	}

	if(refreshFlag_)
	{
		refreshFlag_ = 0;
		if(eventHandler()) appContext_->dispatch(DrawEvent(eventHandler()));
	}
}

void WaylandWindowContext::maximize()
{
    if(wlShellSurface()) wl_shell_surface_set_maximized(wlShellSurface_, nullptr);
	else if(xdgSurface()) xdg_surface_set_maximized(xdgSurface());
}

void WaylandWindowContext::fullscreen()
{
	// TODO: output param?
    if(wlShellSurface())
		wl_shell_surface_set_fullscreen(wlShellSurface(),
			WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 0, nullptr);

	else if(xdgSurface())
		xdg_surface_set_fullscreen(xdgSurface(), nullptr);
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
		xdg_surface_unset_fullscreen(xdgSurface());
		xdg_surface_unset_maximized(xdgSurface());
	}
}
void WaylandWindowContext::beginMove(const MouseButtonEvent* ev)
{
    auto* data = dynamic_cast<WaylandEventData*>(ev->data.get());
    if(!data || !appContext_->wlSeat()) return;

	if(wlShellSurface())
		wl_shell_surface_move(wlShellSurface_, appContext_->wlSeat(), data->serial);
	else if(xdgSurface())
		xdg_surface_move(xdgSurface_, appContext_->wlSeat(), data->serial);
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

wl_display& WaylandWindowContext::wlDisplay() const
{
	return appContext().wlDisplay();
}

void WaylandWindowContext::attachCommit(wl_buffer* buffer)
{
	if(shown_)
	{
		frameCallback_ = wl_surface_frame(wlSurface_);
		wl_callback_add_listener(frameCallback_, &wayland::frameListener, this);

		wl_surface_damage(wlSurface_, 0, 0, size_.x, size_.y);
		wl_surface_attach(wlSurface_, buffer, 0, 0);
	}
	else
	{
		wl_surface_attach(wlSurface_, nullptr, 0, 0);
	}

	wl_surface_commit(wlSurface_);
}

Surface WaylandWindowContext::surface()
{
	return {};
}

}
