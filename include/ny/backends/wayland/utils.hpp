#pragma once

#include <ny/backends/wayland/waylandInclude.hpp>

#include <ny/app/mouse.hpp>
#include <ny/app/keyboard.hpp>
#include <ny/app/cursor.hpp>
#include <ny/backends/backend.hpp>
#include <ny/window/window.hpp>
#include <ny/window/windowEvents.hpp>

#include <wayland-client.h>

#include <iostream>


namespace ny
{


namespace wayland
{

const unsigned int frameEvent = 11;

class waylandFrameEvent : public contextEvent
{
public:
    waylandFrameEvent() : contextEvent(Wayland, frameEvent) {};
};

//surfaceListener////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void shellSurfaceHandlePopupDone(void* data, wl_shell_surface* shellSurface);
void shellSurfaceHandleConfigure(void* data, wl_shell_surface* shellSurface, uint32_t edges, int32_t width, int32_t height);
void shellSurfaceHandlePing(void* data, wl_shell_surface* shellSurface, uint32_t serial);
const wl_shell_surface_listener shellSurfaceListener =
{
    shellSurfaceHandlePing,
    shellSurfaceHandleConfigure,
    shellSurfaceHandlePopupDone
};

//frameCallback///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void surfaceHandleFrame(void* data, wl_callback* callback, uint32_t time);
const wl_callback_listener frameListener =
{
    surfaceHandleFrame
};

//globalRegistry//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void globalRegistryHandleAdd(void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version);
void globalRegistryHandleRemove(void* data, wl_registry* registry, uint32_t id);

const wl_registry_listener globalRegistryListener =
{
    globalRegistryHandleAdd,
    globalRegistryHandleRemove
};

//shm////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void shmHandleFormat(void* data, wl_shm* shm, uint32_t format);
const wl_shm_listener shmListener =
{
    shmHandleFormat
};

//seat/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void seatHandleCapabilities(void* data, wl_seat* seat, unsigned int caps);
const wl_seat_listener seatListener =
{
    seatHandleCapabilities
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//pointer events
void pointerHandleEnter(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy);
void pointerHandleLeave(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface);
void pointerHandleMotion(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
void pointerHandleButton(void* data, wl_pointer* pointer,  uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
void pointerHandleAxis(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value);

const wl_pointer_listener pointerListener =
{
    pointerHandleEnter,
    pointerHandleLeave,
    pointerHandleMotion,
    pointerHandleButton,
    pointerHandleAxis,
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//keyboard events
void keyboardHandleKeymap(void* data, wl_keyboard* keyboard, uint32_t format, int fd, uint32_t size);
void keyboardHandleEnter(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys);
void keyboardHandleLeave(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface);
void keyboardHandleKey(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
void keyboardHandleModifiers(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t modsDepressed, uint32_t modsLatched, uint32_t modsLocked, uint32_t group);

const wl_keyboard_listener keyboardListener =
{
    keyboardHandleKeymap,
    keyboardHandleEnter,
    keyboardHandleLeave,
    keyboardHandleKey,
    keyboardHandleModifiers
};


//display sync///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void displayHandleSync(void* data, wl_callback* callback, uint32_t  time);
const wl_callback_listener displaySyncListener =
{
    displayHandleSync
};

//bufferCreation////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int osCreateAnonymousFile(off_t size);
int createTmpfileCloexec(char *tmpname);
int setCloexecOrClose(int fd);

class shmBuffer
{
protected:
    static const unsigned int defaultSize_ = 1024*1024*10; //10MB

    vec2ui size_;
    wl_buffer* buffer_;
    wl_shm_pool* pool_;
    void* data_;

    void create();
    void destroy();

public:
    shmBuffer(vec2ui size, bufferFormat form = bufferFormat::argb8888);
    ~shmBuffer();

    const bufferFormat format;

    vec2ui getSize() const { return size_; }
    unsigned int getAbsSize() const { return size_.x * getBufferFormatSize(format) * size_.y; }
    wl_buffer* getWlBuffer() const { return buffer_; }
    void* getData(){ return data_; }

    void setSize(const vec2ui& size);
};

}//wayland


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//utils function//////
mouse::button waylandToButton(unsigned int id);
keyboard::key waylandToKey(unsigned int id);

std::string cursorToWayland(cursorType c);
cursorType waylandToCursor(std::string id);

unsigned int bufferFormatToWayland(bufferFormat format);
bufferFormat waylandToBufferFormat(unsigned int wlFormat);

}
