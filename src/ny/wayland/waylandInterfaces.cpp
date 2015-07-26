#include <ny/wayland/waylandInterfaces.hpp>
#include <ny/wayland/waylandAppContext.hpp>
#include <ny/wayland/waylandWindowContext.hpp>
#include <ny/wayland/waylandUtil.hpp>
#include <ny/app.hpp>

#include <ny/wayland/xdg-shell-client-protocol.h>

#include <iostream>

namespace ny
{

namespace wayland
{

//surfaceListener////////////////////////////////////////////////////////////////
void shellSurfaceHandlePing(void* data,  wl_shell_surface *shellSurface, uint32_t serial)
{
    wl_shell_surface_pong(shellSurface, serial);
}

void shellSurfaceHandleConfigure(void* data,  wl_shell_surface *shellSurface, uint32_t edges, int32_t width, int32_t height)
{
    waylandAppContext* a = dynamic_cast<waylandAppContext*>(getMainApp()->getAppContext());

    if(!a)
        return;

    a->eventWindowResized(shellSurface, edges, width, height);
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

//frameCallback//////////////////////////////////////////////////////////////////
void surfaceHandleFrame(void* data,  wl_callback *callback, uint32_t time)
{
    waylandWC* wc =  (waylandWindowContext*)data;

    waylandFrameEvent ev;
    ev.handler = &wc->getWindow();
    ev.backend = Wayland;

    getMainApp()->sendEvent(ev, *ev.handler);
}
const wl_callback_listener frameListener =
{
    surfaceHandleFrame
};

//globalRegistry/////////////////////////////////////////////////////////////////
void globalRegistryHandleAdd(void* data,  wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
{
    waylandAppContext* a = (waylandAppContext*) data;
    a->registryHandler(registry, id, interface, version);
}

void globalRegistryHandleRemove(void* data,  wl_registry* registry, uint32_t id)
{
    waylandAppContext* a = (waylandAppContext*) data;
    a->registryRemover(registry, id);
}

const wl_registry_listener globalRegistryListener =
{
    globalRegistryHandleAdd,
    globalRegistryHandleRemove
};

//shm////////////////////////////////////////////////////////////////////////
void shmHandleFormat(void* data, wl_shm* shm, uint32_t format)
{
    waylandAppContext* a = (waylandAppContext*) data;

    a->shmFormat(shm, format);
}
const wl_shm_listener shmListener =
{
    shmHandleFormat
};

//seat///////////////////////////////////////////////////////////////
void seatHandleCapabilities(void* data,  wl_seat* seat, unsigned int caps)
{
    waylandAppContext* a = (waylandAppContext*) data;
    a->seatCapabilities(seat, caps);
}
const wl_seat_listener seatListener =
{
    seatHandleCapabilities
};

////////////////////////////////////////////////////////////////////////
//pointer events
void pointerHandleEnter(void* data, wl_pointer *pointer, uint32_t serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventMouseEnterSurface(pointer, serial, surface, sx, sy);
}

void pointerHandleLeave(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventMouseLeaveSurface(pointer, serial, surface);
}

void pointerHandleMotion(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventMouseMove(pointer, time, sx, sy);
}

void pointerHandleButton(void* data,  wl_pointer* pointer,  uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventMouseButton(pointer,serial, time, button, state);
}

void pointerHandleAxis(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventMouseAxis(pointer, time, axis, value);
}

const wl_pointer_listener pointerListener =
{
    pointerHandleEnter,
    pointerHandleLeave,
    pointerHandleMotion,
    pointerHandleButton,
    pointerHandleAxis,
};


////////////////////////////////////////////////////////////////////////
//keyboard events
void keyboardHandleKeymap(void* data, wl_keyboard* keyboard, uint32_t format, int fd, uint32_t size)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventKeyboardKeymap(keyboard, format, fd, size);
}

void keyboardHandleEnter(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventKeyboardEnterSurface(keyboard, serial, surface, keys);
}

void keyboardHandleLeave(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventKeyboardLeaveSurface(keyboard, serial, surface);
}

void keyboardHandleKey(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventKeyboardKey(keyboard, serial, time, key, state);
}

void keyboardHandleModifiers(void* data, wl_keyboard* keyboard,uint32_t serial, uint32_t modsDepressed,uint32_t modsLatched, uint32_t modsLocked, uint32_t group)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventKeyboardModifiers(keyboard, serial, modsDepressed, modsLatched, modsLocked, group);
}
const wl_keyboard_listener keyboardListener =
{
    keyboardHandleKeymap,
    keyboardHandleEnter,
    keyboardHandleLeave,
    keyboardHandleKey,
    keyboardHandleModifiers
};


//display sync////////////////////////////////////////////////////////
void displayHandleSync(void* data, wl_callback* callback, uint32_t time)
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
void dataDeviceEnter(void* data, wl_data_device* wl_data_device, unsigned int serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y, wl_data_offer* id)
{
    std::cout << "deviceEnter" << std::endl;
}
void dataDeviceLeave(void* data, wl_data_device* wl_data_device)
{
    std::cout << "deviceLeave" << std::endl;
}
void dataDeviceMotion(void* data, wl_data_device* wl_data_device, unsigned int time, wl_fixed_t x, wl_fixed_t y)
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
void xdgShellPing(void *data, struct xdg_shell *xdg_shell, uint32_t serial)
{

}
const xdg_shell_listener xdgShellListener =
{
    xdgShellPing
};

//xdg-surface
void xdgSurfaceConfigure(void *data, struct xdg_surface *xdg_surface, int32_t width, int32_t height, struct wl_array *states, uint32_t serial)
{
    waylandWindowContext* wc = (waylandWindowContext*) data;

    sizeEvent ev;
    ev.backend = Wayland;
    ev.data = new waylandEventData(serial);
    ev.size = vec2ui(width, height);
    ev.handler = &wc->getWindow();

    getMainApp()->sendEvent(ev, wc->getWindow());
}
void xdgSurfaceClose(void *data, struct xdg_surface *xdg_surface)
{
    waylandWindowContext* wc = (waylandWindowContext*) data;

    destroyEvent ev;
    ev.backend = Wayland;
    ev.handler = &wc->getWindow();

    getMainApp()->sendEvent(ev, wc->getWindow());
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
