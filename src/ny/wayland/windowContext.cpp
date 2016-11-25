// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/input.hpp>
#include <ny/wayland/util.hpp>
#include <ny/wayland/xdg-shell-client-protocol.h>

#include <ny/common/unix.hpp>
#include <ny/mouseContext.hpp>
#include <ny/cursor.hpp>
#include <ny/log.hpp>

#include <nytl/vecOps.hpp>

#include <wayland-cursor.h>

#include <iostream>
#include <cstring>

namespace ny
{

WaylandWindowContext::WaylandWindowContext(WaylandAppContext& ac,
	const WaylandWindowSettings& settings) : appContext_(&ac)
{
    wlSurface_ = wl_compositor_create_surface(&ac.wlCompositor());
    if(!wlSurface_)
		throw std::runtime_error("ny::WaylandWindowContext: could not create wl_surface");

    wl_surface_set_user_data(wlSurface_, this);

    //parse settings
	size_ = settings.size;
    if(nytl::allEqual(size_, defaultSize)) size_ = fallbackSize;
	shown_ = settings.show;
    if(settings.listener) listener(*settings.listener);

    //surface
    if(settings.nativeHandle)
    {
        //In this case we simply copy the surface and not create/handle any role for it
        //TODO: use WaylandWindowSettings for a role on a provided handle
        wlSurface_ = settings.nativeHandle.asPtr<wl_surface>();
    }
    else if(settings.parent.pointer()) //subsurface
	{
		auto& parent = *reinterpret_cast<wl_surface*>(settings.parent.pointer());
		createSubsurface(parent, settings);
	}
	else //toplevel shell surface
	{
		if(ac.xdgShell()) createXDGSurface(settings);
		else if(ac.wlShell()) createShellSurface(settings);
		else throw std::runtime_error("ny::WaylandWindowContext: compositor has no shell global");

		switch(settings.initState)
		{
			case ToplevelState::normal: normalState(); break;
			case ToplevelState::fullscreen: fullscreen(); break;
			case ToplevelState::maximized: maximize(); break;
			case ToplevelState::minimized: minimize(); break;
			default: break;
		}
	}

	cursor(settings.cursor);
	if(settings.show) show();
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
	using WWC = WaylandWindowContext;
	constexpr static wl_shell_surface_listener shellSurfaceListener = {
		memberCallback<decltype(&WWC::handleShellSurfacePing), &WWC::handleShellSurfacePing,
			void(wl_shell_surface*, uint32_t)>,
		memberCallback<decltype(&WWC::handleShellSurfaceConfigure),
			&WWC::handleShellSurfaceConfigure, void(wl_shell_surface*, uint32_t, int32_t, int32_t)>,
		memberCallback<decltype(&WWC::handleShellSurfacePopupDone),
			&WWC::handleShellSurfacePopupDone, void(wl_shell_surface*)>
	};

    wlShellSurface_ = wl_shell_get_shell_surface(appContext().wlShell(), wlSurface_);
    if(!wlShellSurface_)
		throw std::runtime_error("ny::WaylandWindowContext: failed to create wl_shell_surface");

    role_ = WaylandSurfaceRole::shell;

    wl_shell_surface_set_user_data(wlShellSurface_, this);
    wl_shell_surface_add_listener(wlShellSurface_, &shellSurfaceListener, this);

	wl_shell_surface_set_title(wlShellSurface_, ws.title.c_str());
	wl_shell_surface_set_class(wlShellSurface_, "ny::application"); //TODO: AppContextSettings
}

void WaylandWindowContext::createXDGSurface(const WaylandWindowSettings& ws)
{
	using WWC = WaylandWindowContext;
	constexpr static xdg_surface_listener xdgSurfaceListener = {
		memberCallback<decltype(&WWC::handleXdgSurfaceConfigure), &WWC::handleXdgSurfaceConfigure,
			void(xdg_surface*, int32_t, int32_t, wl_array*, uint32_t)>,
		memberCallback<decltype(&WWC::handleXdgSurfaceClose), &WWC::handleXdgSurfaceClose,
			void(xdg_surface*)>
	};

    xdgSurface_ = xdg_shell_get_xdg_surface(appContext().xdgShell(), wlSurface_);
    if(!xdgSurface_)
		throw std::runtime_error("ny::WaylandWindowContext: failed to create xdg_surface");

    role_ = WaylandSurfaceRole::xdg;

	xdg_surface_set_window_geometry(xdgSurface_, 0, 0, size_.x, size_.y);
    xdg_surface_set_user_data(xdgSurface_, this);
    xdg_surface_add_listener(xdgSurface_, &xdgSurfaceListener, this);

	xdg_surface_set_title(xdgSurface_, ws.title.c_str());
	xdg_surface_set_app_id(xdgSurface_, "ny::application"); //TODO: AppContextSettings
}

void WaylandWindowContext::createSubsurface(wl_surface& parent, const WaylandWindowSettings&)
{
	auto subcomp = appContext().wlSubcompositor();
    if(!subcomp) throw std::runtime_error("ny::WaylandWindowContext: no wl_subcompositor");

    role_ = WaylandSurfaceRole::sub;
    wlSubsurface_ = wl_subcompositor_get_subsurface(subcomp, wlSurface_, &parent);

    if(!wlSubsurface_)
		throw std::runtime_error("ny::WaylandWindowContext: failed to create wl_subsurface");

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

	appContext().dispatch(&listener(),
		[](WindowListener* listener){ listener->draw(nullptr); });
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

void WaylandWindowContext::size(nytl::Vec2ui size)
{
	size_ = size;
	refresh();
}

void WaylandWindowContext::position(nytl::Vec2i position)
{
    if(wlSubsurface())
    {
		wl_subsurface_set_position(wlSubsurface_, position.x, position.y);
    }
	else
	{
		warning("ny::WaylandWindowContext::position: wayland does not support custom positions");
	}
}

void WaylandWindowContext::cursor(const Cursor& cursor)
{
	auto wmc = appContext().waylandMouseContext();
	if(!wmc)
	{
		warning("ny::WaylandWindowContext::cursor: no WaylandMouseContext");
		return;
	}

	if(cursor.type() == Cursor::Type::image)
	{
		auto* img = cursor.image();
		if(!img || !img->data)
		{
			warning("ny::WaylandWindowContext::cursor: invalid image cursor");
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
			warning("ny::WaylandWindowContext::cursor: failed to retrieve cursor ", cursorName);
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

	//update the cursor if needed
	if(wmc->over() == this)
		wmc->cursorBuffer(cursorBuffer_, cursorHotspot_, cursorSize_);
}

// void WaylandWindowContext::droppable(const DataTypes&)
// {
// 	//TODO
// 	//currently all windows are droppabe, store it here and check it in wayland/data.cpp
// 	warning("ny::WaylandWindowContext::droppable: not implemented");
// }

void WaylandWindowContext::minSize(nytl::Vec2ui)
{
	warning("ny::WaylandWindowContext::maxSize: wayland has no capability for size limits");
}
void WaylandWindowContext::maxSize(nytl::Vec2ui)
{
	warning("ny::WaylandWindowContext::maxSize: wayland has no capability for size limits");
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

void WaylandWindowContext::maximize()
{
    if(wlShellSurface()) wl_shell_surface_set_maximized(wlShellSurface_, nullptr);
	else if(xdgSurface()) xdg_surface_set_maximized(xdgSurface());
}

void WaylandWindowContext::fullscreen()
{
	// TODO: which wayland output to choose here?
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
void WaylandWindowContext::beginMove(const EventData* ev)
{
    auto* data = dynamic_cast<const WaylandEventData*>(ev);
    if(!data || !appContext().wlSeat()) return;

	if(wlShellSurface())
		wl_shell_surface_move(wlShellSurface_, appContext().wlSeat(), data->serial);
	else if(xdgSurface())
		xdg_surface_move(xdgSurface_, appContext().wlSeat(), data->serial);
}

void WaylandWindowContext::beginResize(const EventData* ev, WindowEdges edge)
{
    auto* data = dynamic_cast<const WaylandEventData*>(ev);

    if(!data || !appContext().wlSeat()) return;

    // unsigned int wlEdge = static_cast<unsigned int>(edge);
    unsigned int wlEdge = edge.value();
    wl_shell_surface_resize(wlShellSurface_, appContext().wlSeat(), data->serial, wlEdge);
}

void WaylandWindowContext::title(nytl::StringParam titlestring)
{
	if(wlShellSurface_) wl_shell_surface_set_title(wlShellSurface_, titlestring);
	else if(xdgSurface_) xdg_surface_set_title(xdgSurface_, titlestring);
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
	using WWC = WaylandWindowContext;
	static constexpr wl_callback_listener frameListener = {
		memberCallback<decltype(&WWC::handleFrameCallback),
			&WWC::handleFrameCallback, void(wl_callback*, uint32_t)>
	};

	if(shown_)
	{
		frameCallback_ = wl_surface_frame(wlSurface_);
		wl_callback_add_listener(frameCallback_, &frameListener, this);

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

void WaylandWindowContext::handleFrameCallback()
{
	if(frameCallback_)
	{
		wl_callback_destroy(frameCallback_);
		frameCallback_ = nullptr;
	}

	if(refreshFlag_)
	{
		refreshFlag_ = false;
		appContext().dispatch(&listener(),
			[](WindowListener* listener) { listener->draw(nullptr); });
	}
}

void WaylandWindowContext::handleShellSurfacePing(unsigned int serial)
{
    wl_shell_surface_pong(wlShellSurface(), serial);
}

void WaylandWindowContext::handleShellSurfaceConfigure(unsigned int edges, int width, int height)
{
	nytl::unused(edges);

	auto newSize = nytl::Vec2ui(width, height);
	appContext().dispatch(&listener(),
		[=](WindowListener* listener) { listener->resize(newSize, nullptr); });

	size(newSize);
}

void WaylandWindowContext::handleShellSurfacePopupDone()
{
	//TODO
}

void WaylandWindowContext::handleXdgSurfaceConfigure(int width, int height, wl_array* states,
	unsigned int serial)
{
	//TODO
	if(!width || !height) return;
	nytl::unused(states);

	auto newSize = nytl::Vec2ui(width, height);
	appContext().dispatch(&listener(),
		[=](WindowListener* listener) { listener->resize(newSize, nullptr); });

	xdg_surface_ack_configure(xdgSurface(), serial);
	size(newSize);
}

void WaylandWindowContext::handleXdgSurfaceClose()
{
	appContext().dispatch(&listener(),
		[=](WindowListener* listener) { listener->close(nullptr); });
}

void WaylandWindowContext::handleXdgPopupDone()
{
	//TODO
}

}
