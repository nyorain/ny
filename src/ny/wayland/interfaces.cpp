#include <ny/wayland/interfaces.hpp>
#include <ny/wayland/util.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/input.hpp>
#include <ny/events.hpp>

#include <ny/wayland/xdg-shell-client-protocol.h>
#include <wayland-client-protocol.h>

#include <iostream>

namespace ny
{

namespace wayland
{

//surface
void shellSurfaceHandlePing(void*, wl_shell_surface* shellSurface, uint32_t serial)
{
    wl_shell_surface_pong(shellSurface, serial);
}

void shellSurfaceHandleConfigure(void* data, wl_shell_surface*, uint32_t edges, 
	int32_t width, int32_t height)
{
	auto wc = static_cast<WaylandWindowContext*>(data);
    if(!wc) return;
	
	wc->configureEvent(nytl::Vec2ui(width, height), static_cast<WindowEdge>(edges));
}

void shellSurfaceHandlePopupDone(void* data, wl_shell_surface *shellSurface)
{
}

const wl_shell_surface_listener shellSurfaceListener =
{
    shellSurfaceHandlePing,
    shellSurfaceHandleConfigure,
    shellSurfaceHandlePopupDone
};

//frameCallback
void surfaceHandleFrame(void* data, wl_callback* callback, uint32_t)
{
    auto* wc = static_cast<WaylandWindowContext*>(data);
	if(!wc) return;

	wc->frameEvent();
}
const wl_callback_listener frameListener =
{
    surfaceHandleFrame
};

//globalRegistry
void globalRegistryHandleAdd(void* data, wl_registry*, uint32_t id, 
	const char* interface, uint32_t version)
{
    auto* ac = static_cast<WaylandAppContext*>(data);
    ac->registryAdd(id, interface, version);
}

void globalRegistryHandleRemove(void* data,  wl_registry*, uint32_t id)
{
    auto* ac = static_cast<WaylandAppContext*>(data);
    ac->registryRemove(id);
}

const wl_registry_listener globalRegistryListener =
{
    globalRegistryHandleAdd,
    globalRegistryHandleRemove
};

//shm
void shmHandleFormat(void* data, wl_shm*, uint32_t format)
{
    auto* ac = static_cast<WaylandAppContext*>(data);
    ac->addShmFormat(format);
}

const wl_shm_listener shmListener =
{
    shmHandleFormat
};

//seat
void seatHandleCapabilities(void* data, wl_seat*, unsigned int caps)
{
    auto* ac = static_cast<WaylandAppContext*>(data);
    ac->seatCapabilities(caps);
}
void seatName(void *data, struct wl_seat*, const char* name)
{
    auto* ac = static_cast<WaylandAppContext*>(data);
    ac->seatName(name);
}
const wl_seat_listener seatListener =
{
    seatHandleCapabilities,
	seatName
};

//xdg-shell
void xdgShellPing(void* data, struct xdg_shell* shell, uint32_t serial)
{
	xdg_shell_pong(shell, serial);
}

const xdg_shell_listener xdgShellListener =
{
    xdgShellPing
};

//xdg-surface
void xdgSurfaceConfigure(void *data, struct xdg_surface *xdg_surface, int32_t width, 
	int32_t height, struct wl_array *states, uint32_t serial)
{
    auto* wc = static_cast<WaylandWindowContext*>(data);
    if(!wc) return;

	wc->configureEvent(nytl::Vec2ui(width, height), static_cast<WindowEdge>(0u));
}

void xdgSurfaceClose(void *data, struct xdg_surface *xdg_surface)
{
    auto* wc = static_cast<WaylandWindowContext*>(data);
	auto* eh = wc->eventHandler();
    if(!eh) return;
    wc->appContext().dispatch(CloseEvent(eh));
}
const xdg_surface_listener xdgSurfaceListener =
{
    xdgSurfaceConfigure,
    xdgSurfaceClose
};

//xdg-popup
void xdgPopupDone(void *data, struct xdg_popup *xdg_popup)
{

}
const xdg_popup_listener xdgPopupListener =
{
    xdgPopupDone
};

} //namespace wayland

} //namespace ny
