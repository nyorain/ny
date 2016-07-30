#pragma once

#include <ny/include.hpp>

#include <ny/app/mouse.hpp>
#include <ny/app/keyboard.hpp>
#include <ny/window/cursor.hpp>

#include <nytl/vec.hpp>

#include <wayland-client-protocol.h>

#include <iostream>
#include <atomic>


namespace ny
{

//wayland specific events
namespace eventType
{
	namespace wayland
	{
		constexpr auto frameEvent = 1001u;
		constexpr auto configureEvent = 1002u;
	}
}

///Used for e.g. move/resize requests where the serial of the trigger must be given
class WaylandEventData : public ny::EventData
{
public:
    WaylandEventData(unsigned int xserial) : serial(xserial) {};
    unsigned int serial;
};


//wayland
namespace wayland
{

//events
///This Event will be sent to a WaylandWindowContext if its frame callback was triggered.
using FrameEvent = EventBase<eventType::wayland::frameEvent>;

class ConfigureEvent : public EventBase<eventType::wayland::configureEvent>
{
public:
	using EvBase::EvBase;
	ny::WindowEdge edges;
	nytl::Vec2ui size;
};

///Defines a wayland shm buffer that can be resized.
class ShmBuffer
{
public:
    ShmBuffer(Vec2ui size);
    ~ShmBuffer();

    Vec2ui getSize() const { return size_; }
    unsigned int dataSize() const { return size_.x * 4 * size_.y; }
    wl_buffer* wlBuffer() const { return buffer_; }
	std::uint8_t* data(){ return data_; }

    void use() { used_.store(1); }
    bool used() const { return used_.load(); }

    void size(const Vec2ui& size);

protected:
    //static const unsigned int defaultSize_ = 1024 * 1024 * 5; //5MB
    unsigned int shmSize_ = 1024 * 1024 * 5; //5MB

    Vec2ui size_;
    wl_buffer* buffer_;
    wl_shm_pool* pool_;
	std::uint8_t* data_;

    std::atomic<bool> used_ {0};

    void create();
    void destroy();

    //release cb
    friend void bufferRelease(void*, wl_buffer*);
    void wasReleased(){ used_.store(0); }

};

//serverCallback
class ServerCallback
{
public:
	ServerCallback(wl_callback* callback);
    Callback<void(wl_callback*, unsigned int)> callback;

protected:
    friend void CallbackDone(void*, struct wl_Callback*, uint32_t);
    void done(wl_Callback*, unsigned int data);
};

///Holds information about a wayland output.
class Output
{

friend void outputGeometry(void*, wl_output*, int, int, int, int, int, const char*, const char*, int);
friend void outputMode(void*, wl_output*, unsigned int, int, int, int);
friend void outputDone(void*, wl_output*);
friend void outputScale(void*, wl_output*, int);

protected:
    wl_output* wlOutput_ = nullptr;

    Vec2i position_;
    Vec2i size_;
    Vec2i physicalSize_;

    int subpixel_;
    int refreshRate_;

    unsigned int flags_;

    std::string make_;
    std::string model_;

    int transform_;
    int scale_;

public:
    Output(wl_output& outp);
    ~Output();

    Vec2i getPosition() const { return position_; }
    Vec2i getSize() const { return size_; }
    Vec2i getPhysicalSize() const { return physicalSize_; }

    int getSubpixel() const { return subpixel_; }
    int getRefreshRate() const { return refreshRate_; }

    unsigned int getFlags() const { return flags_; }

    std::string getMake() const { return make_; }
    std::string getModel() const { return model_; }

    int getTransform() const { return transform_; }
    int getScale() const { return scale_; }

    wl_output* getWlOutput() const { return wlOutput_; }
};

}//wayland

//convert function
MouseButton linuxToButton(unsigned int id);
Key linuxToKey(unsigned int id);

std::string cursorToWayland(Cursor::Type type);
Cursor::Type waylandToCursor(std::string id);

}
