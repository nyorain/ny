#include <ny/backend/wayland/interfaces.hpp>
#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/input.hpp>
#include <ny/backend/wayland/util.hpp>

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
	
	ConfigureEvent event(wc);
	event.edges = waylandToEdges(edges);
	event.size = nytl::Vec2ui(width, height);
	wc->appContext().sendEvent(std::move(event));
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
void surfaceHandleFrame(void* data, wl_Callback* callback, uint32_t time)
{
    auto* wc = static_cast<WaylandWindowContext*>(data);
    nyMainApp()->sendEvent(make_unique<waylandFrameEvent>(&wc->getWindow()));
}
const wl_Callback_listener frameListener =
{
    surfaceHandleFrame
};

//globalRegistry
void globalRegistryHandleAdd(void* data, wl_registry* registry, uint32_t id, 
	const char* interface, uint32_t version)
{
    auto* ac = static_cast<WaylandAppContext*>(data);
    ac->registryAdd(id, interface, version);
}

void globalRegistryHandleRemove(void* data,  wl_registry* registry, uint32_t id)
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
void shmHandleFormat(void* data, wl_shm* shm, uint32_t format)
{
    auto* ac = static_cast<WaylandAppContext*>(data);
    ac->shmFormat(shm, format);
}

const wl_shm_listener shmListener =
{
    shmHandleFormat
};

//seat
void seatHandleCapabilities(void* data, wl_seat* seat, unsigned int caps)
{
    auto* ac = static_cast<WaylandAppContext*>(data);
    ac->seatCapabilities(caps);
}
const wl_seat_listener seatListener =
{
    seatHandleCapabilities
};

//pointer events
void pointerHandleEnter(void* data, wl_pointer *pointer, uint32_t serial, wl_surface* surface, 
	wl_fixed_t sx, wl_fixed_t sy)
{
    auto* mc = static_cast<WaylandMouseContext*>(data);
	Vec2ui pos(wl_fixed_to_int(sx), wl_fixed_to_int(sy));
    mc->handleEnter(serial, *surface, pos);
}

void pointerHandleLeave(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface)
{
    auto* mc = static_cast<WaylandMouseContext*>(data);
	mc->handleLeave(serial, *surface);
}

void pointerHandleMotion(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t sx, 
	wl_fixed_t sy)
{
    auto* mc = static_cast<WaylandMouseContext*>(data);
	Vec2ui pos(wl_fixed_to_int(sx), wl_fixed_to_int(sy));
	mc->handleMotion(time, pos);
}

void pointerHandleButton(void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, 
	uint32_t button, uint32_t state)
{
    auto* mc = static_cast<WaylandMouseContext*>(data);
	mv->handleButton(serial, time, button, state);
}

void pointerHandleAxis(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, 
	wl_fixed_t value)
{
    auto* mc = static_cast<WaylandMouseContext*>(data);
    app->handleAxis(time, axis, value);
}

const wl_pointer_listener pointerListener =
{
    pointerHandleEnter,
    pointerHandleLeave,
    pointerHandleMotion,
    pointerHandleButton,
    pointerHandleAxis,
};


//keyboard events
void keyboardHandleKeymap(void* data, wl_keyboard* kb, uint32_t format, int fd, uint32_t size)
{
	auto* kc = static_cast<WaylandKeyboardContext*>(data);
    kc->handleKeymap(format, fd, size);
}

void keyboardHandleEnter(void* data, wl_keyboard* keyboard, uint32_t serial, 
	wl_surface* surface, wl_array* keys)
{
	auto* kc = static_cast<WaylandKeyboardContext*>(data);
    kc->handleEnter(serial, *surface, keys);
}

void keyboardHandleLeave(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface)
{
	auto* kc = static_cast<WaylandKeyboardContext*>(data);
    kc->handleLeave(serial, *surface);
}

void keyboardHandleKey(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, 
	uint32_t key, uint32_t state)
{
	auto* kc = static_cast<WaylandKeyboardContext*>(data);
    kc->handleKey(serial, time, key, state);
}

void keyboardHandleModifiers(void* data, wl_keyboard* keyboard, uint32_t serial, 
	uint32_t modsDepressed, uint32_t modsLatched, uint32_t modsLocked, uint32_t group)
{
	auto* kc = static_cast<WaylandKeyboardContext*>(data);
    kc->handleModifiers(serial, modsDepressed, modsLatched, modsLocked, group);
}

const wl_keyboard_listener keyboardListener =
{
    keyboardHandleKeymap,
    keyboardHandleEnter,
    keyboardHandleLeave,
    keyboardHandleKey,
    keyboardHandleModifiers
};


//display sync
void displayHandleSync(void* data, wl_Callback* Callback, uint32_t time)
{
}

const wl_callback_listener displaySyncListener =
{
    displayHandleSync
};

//dataSourceListener
void dataSourceTarget(void* data, wl_data_source* wl_data_source, const char* mime_type)
{
    std::cout << "target" << std::endl;
}
void dataSourceSend(void* data, wl_data_source* wl_data_source, const char* mime_type, int fd)
{
    std::cout << "send" << std::endl;
}
void dataSourceCancelled(void* data, wl_data_source* wl_data_source)
{
    std::cout << "cancelled" << std::endl;
}
const wl_data_source_listener dataSourceListener =
{
    dataSourceTarget,
    dataSourceSend,
    dataSourceCancelled
};

//dataOffer
void dataOfferOffer(void* data, wl_data_offer* wl_data_offer, const char* mime_type)
{
    std::cout << "offer" << std::endl;
}

const wl_data_offer_listener dataOfferListener =
{
    dataOfferOffer
};

//dataDevice
void dataDeviceOffer(void* data, wl_data_device* wl_data_device, wl_data_offer* id)
{
     std::cout << "deviceOffer" << std::endl;
}
void dataDeviceEnter(void* data, wl_data_device* wl_data_device, unsigned int serial, 
	wl_surface* surface, wl_fixed_t x, wl_fixed_t y, wl_data_offer* id)
{
    std::cout << "deviceEnter" << std::endl;
}
void dataDeviceLeave(void* data, wl_data_device* wl_data_device)
{
    std::cout << "deviceLeave" << std::endl;
}
void dataDeviceMotion(void* data, wl_data_device* wl_data_device, unsigned int time, 
	wl_fixed_t x, wl_fixed_t y)
{
    std::cout << "deviceMotion" << std::endl;
}
void dataDeviceDrop(void* data, wl_data_device* wl_data_device)
{
    std::cout << "deviceDrop" << std::endl;
}
void dataDeviceSelection(void* data, wl_data_device* wl_data_device, wl_data_offer* id)
{
    std::cout << "deviceSelection" << std::endl;
}
const wl_data_device_listener dataDeviceListener =
{
    dataDeviceOffer,
    dataDeviceEnter,
    dataDeviceLeave,
    dataDeviceMotion,
    dataDeviceDrop,
    dataDeviceSelection
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
    waylandWindowContext* wc = (waylandWindowContext*) data;
    nyMainApp()->sendEvent(make_unique<sizeEvent>(&wc->getWindow(), 
		Vec2ui(width, height), 1, new waylandEventData(serial)));
}

void xdgSurfaceClose(void *data, struct xdg_surface *xdg_surface)
{
    waylandWindowContext* wc = (waylandWindowContext*) data;
    nyMainApp()->sendEvent(make_unique<destroyEvent>(&wc->getWindow()));
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
