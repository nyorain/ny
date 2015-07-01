#pragma once

#include <ny/wayland/waylandInclude.hpp>

#include <ny/mouse.hpp>
#include <ny/keyboard.hpp>
#include <ny/cursor.hpp>
#include <ny/backend.hpp>
#include <ny/window.hpp>
#include <ny/windowEvents.hpp>

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

//surfaceListener////////////////////////////////////////////////////////////////
void shellSurfaceHandlePopupDone(void* data, wl_shell_surface* shellSurface);
void shellSurfaceHandleConfigure(void* data, wl_shell_surface* shellSurface, unsigned int edges, int width, int height);
void shellSurfaceHandlePing(void* data, wl_shell_surface* shellSurface, unsigned int serial);
const wl_shell_surface_listener shellSurfaceListener =
{
    shellSurfaceHandlePing,
    shellSurfaceHandleConfigure,
    shellSurfaceHandlePopupDone
};

//frameCallback//////////////////////////////////////////////////////////////////
void surfaceHandleFrame(void* data, wl_callback* callback, unsigned int time);
const wl_callback_listener frameListener =
{
    surfaceHandleFrame
};

//globalRegistry/////////////////////////////////////////////////////////////////
void globalRegistryHandleAdd(void* data, wl_registry* registry, unsigned int id, const char* interface, unsigned int version);
void globalRegistryHandleRemove(void* data, wl_registry* registry, unsigned int id);

const wl_registry_listener globalRegistryListener =
{
    globalRegistryHandleAdd,
    globalRegistryHandleRemove
};

//shm////////////////////////////////////////////////////////////////////////
void shmHandleFormat(void* data, wl_shm* shm, unsigned int format);
const wl_shm_listener shmListener =
{
    shmHandleFormat
};

//seat///////////////////////////////////////////////////////////////
void seatHandleCapabilities(void* data, wl_seat* seat, unsigned int caps);
const wl_seat_listener seatListener =
{
    seatHandleCapabilities
};

////////////////////////////////////////////////////////////////////////
//pointer events
void pointerHandleEnter(void* data, wl_pointer* pointer, unsigned int serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy);
void pointerHandleLeave(void* data, wl_pointer* pointer, unsigned int serial, wl_surface* surface);
void pointerHandleMotion(void* data, wl_pointer* pointer, unsigned int time, wl_fixed_t sx, wl_fixed_t sy);
void pointerHandleButton(void* data, wl_pointer* pointer,  unsigned int serial, unsigned int time, unsigned int button, unsigned int state);
void pointerHandleAxis(void* data, wl_pointer* pointer, unsigned int time, unsigned int axis, wl_fixed_t value);

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
void keyboardHandleKeymap(void* data, wl_keyboard* keyboard, unsigned int format, int fd, unsigned int size);
void keyboardHandleEnter(void* data, wl_keyboard* keyboard, unsigned int serial, wl_surface* surface, wl_array* keys);
void keyboardHandleLeave(void* data, wl_keyboard* keyboard, unsigned int serial, wl_surface* surface);
void keyboardHandleKey(void* data, wl_keyboard* keyboard, unsigned int serial, unsigned int time, unsigned int key, unsigned int state);
void keyboardHandleModifiers(void* data, wl_keyboard* keyboard, unsigned int serial, unsigned int modsDepressed, unsigned int modsLatched, unsigned int modsLocked, unsigned int group);

const wl_keyboard_listener keyboardListener =
{
    keyboardHandleKeymap,
    keyboardHandleEnter,
    keyboardHandleLeave,
    keyboardHandleKey,
    keyboardHandleModifiers
};


//display sync////////////////////////////////////////////////////////
void displayHandleSync(void* data, wl_callback* callback, unsigned int  time);
const wl_callback_listener displaySyncListener =
{
    displayHandleSync
};

//dataSourceListener
void dataSourceTarget(void* data, wl_data_source* wl_data_source, const char* mime_type);
void dataSourceSend(void* data, wl_data_source* wl_data_source, const char* mime_type, int fd);
void dataSourceCancelled(void* data, wl_data_source* wl_data_source);
const wl_data_source_listener dataSourceListener =
{
    dataSourceTarget,
    dataSourceSend,
    dataSourceCancelled
};

//dataOffer
void dataOfferOffer(void* data, wl_data_offer* wl_data_offer, const char* mime_type);
const wl_data_offer_listener dataOfferListener =
{
    dataOfferOffer
};

//dataDevice
void dataDeviceOffer(void* data, wl_data_device* wl_data_device, wl_data_offer* id);
void dataDeviceEnter(void* data, wl_data_device* wl_data_device, unsigned int serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y, wl_data_offer* id);
void dataDeviceLeave(void* data, wl_data_device* wl_data_device);
void dataDeviceMotion(void* data, wl_data_device* wl_data_device, unsigned int time, wl_fixed_t x, wl_fixed_t y);
void dataDeviceDrop(void* data, wl_data_device* wl_data_device);
void dataDeviceSelection(void* data, wl_data_device* wl_data_device, wl_data_offer* id);
const wl_data_device_listener dataDeviceListener =
{
    dataDeviceOffer,
    dataDeviceEnter,
    dataDeviceLeave,
    dataDeviceMotion,
    dataDeviceDrop,
    dataDeviceSelection
};

//output
void outputGeometry(void* data, wl_output* wl_output, int x, int y, int physical_width, int physical_height, int subpixel, const char* make, const char* model, int transform);
void outputMode(void* data, wl_output* wl_output, unsigned int flags, int width, int height, int refresh);
void outputDone(void* data, wl_output* wl_output);
void outputScale(void* data, wl_output* wl_output, int factor);
const wl_output_listener outputListener =
{
    outputGeometry,
    outputMode,
    outputDone,
    outputScale
};

//bufferCreation//////////////////////////////////////////////////////////////
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


/////////////////////////////////////////////////////////////////////////
//utils function///
mouse::button waylandToButton(unsigned int id);
keyboard::key waylandToKey(unsigned int id);

std::string cursorToWayland(cursorType c);
cursorType waylandToCursor(std::string id);

unsigned int bufferFormatToWayland(bufferFormat format);
bufferFormat waylandToBufferFormat(unsigned int wlFormat);

}
