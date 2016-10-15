#include <ny/backend/wayland/interfaces.hpp>
#include <ny/backend/wayland/util.hpp>
#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/input.hpp>
#include <ny/backend/events.hpp>

#include <ny/backend/wayland/xdg-shell-client-protocol.h>
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
	event.edge = waylandToEdge(edges);
	event.size = nytl::Vec2ui(width, height);
	wc->appContext().dispatch(std::move(event));
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
	FrameEvent ev(wc);
	ev.wlCallback = callback;
    wc->appContext().dispatch(std::move(ev));
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

//pointer events
void pointerHandleEnter(void* data, wl_pointer*, uint32_t serial, wl_surface* surface, 
	wl_fixed_t sx, wl_fixed_t sy)
{
    auto* mc = static_cast<WaylandMouseContext*>(data);
	Vec2ui pos(wl_fixed_to_int(sx), wl_fixed_to_int(sy));
    mc->handleEnter(serial, *surface, pos);
}

void pointerHandleLeave(void* data, wl_pointer*, uint32_t serial, wl_surface* surface)
{
    auto* mc = static_cast<WaylandMouseContext*>(data);
	mc->handleLeave(serial, *surface);
}

void pointerHandleMotion(void* data, wl_pointer*, uint32_t time, wl_fixed_t sx, 
	wl_fixed_t sy)
{
    auto* mc = static_cast<WaylandMouseContext*>(data);
	Vec2ui pos(wl_fixed_to_int(sx), wl_fixed_to_int(sy));
	mc->handleMotion(time, pos);
}

void pointerHandleButton(void* data, wl_pointer*, uint32_t serial, uint32_t time, 
	uint32_t button, uint32_t state)
{
    auto* mc = static_cast<WaylandMouseContext*>(data);
	mc->handleButton(serial, time, button, state);
}

void pointerHandleAxis(void* data, wl_pointer*, uint32_t time, uint32_t axis, 
	wl_fixed_t value)
{
    auto* mc = static_cast<WaylandMouseContext*>(data);
    mc->handleAxis(time, axis, value);
}

void pointerHandleFrame(void *data, struct wl_pointer *wl_pointer)
{
}
void pointerHandleAxisSource(void *data, struct wl_pointer *wl_pointer, uint32_t axis_source)
{
}
void pointerHandleAxisStop(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis)
{
}
void pointerHandleAxisDiscrete(void *data, struct wl_pointer*, uint32_t axis, int32_t discrete)
{
}
const wl_pointer_listener pointerListener =
{
    pointerHandleEnter,
    pointerHandleLeave,
    pointerHandleMotion,
    pointerHandleButton,
    pointerHandleAxis,
	pointerHandleFrame,
	pointerHandleAxisSource,
	pointerHandleAxisStop,
	pointerHandleAxisDiscrete
};


//keyboard events
void keyboardHandleKeymap(void* data, wl_keyboard*, uint32_t format, int fd, uint32_t size)
{
	auto* kc = static_cast<WaylandKeyboardContext*>(data);
    kc->handleKeymap(format, fd, size);
}

void keyboardHandleEnter(void* data, wl_keyboard*, uint32_t serial, 
	wl_surface* surface, wl_array* keys)
{
	auto* kc = static_cast<WaylandKeyboardContext*>(data);
    kc->handleEnter(serial, *surface, *keys);
}

void keyboardHandleLeave(void* data, wl_keyboard*, uint32_t serial, wl_surface* surface)
{
	auto* kc = static_cast<WaylandKeyboardContext*>(data);
    kc->handleLeave(serial, *surface);
}

void keyboardHandleKey(void* data, wl_keyboard*, uint32_t serial, uint32_t time, 
	uint32_t key, uint32_t state)
{
	auto* kc = static_cast<WaylandKeyboardContext*>(data);
    kc->handleKey(serial, time, key, state);
}

void keyboardHandleModifiers(void* data, wl_keyboard*, uint32_t serial, 
	uint32_t modsDepressed, uint32_t modsLatched, uint32_t modsLocked, uint32_t group)
{
	auto* kc = static_cast<WaylandKeyboardContext*>(data);
    kc->handleModifiers(serial, modsDepressed, modsLatched, modsLocked, group);
}

void keyboardHandleRepeatInfo(void* data, struct wl_keyboard*, int32_t rate, int32_t delay)
{
}

const wl_keyboard_listener keyboardListener =
{
    keyboardHandleKeymap,
    keyboardHandleEnter,
    keyboardHandleLeave,
    keyboardHandleKey,
    keyboardHandleModifiers,
	keyboardHandleRepeatInfo
};


//dataSourceListener
void dataSourceTarget(void* data, wl_data_source* dataSource, const char* mimeType)
{
    std::cout << "target " << mimeType << std::endl;
}
void dataSourceSend(void* data, wl_data_source* dataSource, const char* mimeType, int fd)
{
    std::cout << "send " << mimeType << std::endl;
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
    auto* wc = static_cast<WaylandWindowContext*>(data);
	auto* eh = wc->eventHandler();
    if(!eh) return;

	SizeEvent event(eh, new WaylandEventData(serial));
	event.size = Vec2ui(width, height);
  
	wc->appContext().dispatch(std::move(event));
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
