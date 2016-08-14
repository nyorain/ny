#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/mouseContext.hpp>
#include <ny/backend/keyboardContext.hpp>
#include <ny/base/cursor.hpp>

#include <nytl/vec.hpp>

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

///This Event will be sent to a WaylandWindowContext if it holds a surface with shell
///(wayland or xdg) role that is configured. Holds new size and resized edge.
class ConfigureEvent : public EventBase<eventType::wayland::configureEvent>
{
public:
	using EvBase::EvBase;
	ny::WindowEdge edges;
	nytl::Vec2ui size;
};

//TODO: use one shared shm_pool (or maybe one per thread or sth.)
///Defines a wayland shm buffer that can be resized.
class ShmBuffer
{
public:
    ShmBuffer(WaylandAppContext& ac, Vec2ui size);
    ~ShmBuffer();

    Vec2ui size() const { return size_; }
    unsigned int dataSize() const { return size_.x * 4 * size_.y; }
	std::uint8_t& data(){ return *data_; }
    wl_buffer& wlBuffer() const { return *buffer_; }

    void use() { used_.store(1); }
    bool used() const { return used_.load(); }

	///Will trigger a buffer recreate if the given size exceeds the current size.
	///At the beginning 5MB will be allocated for a buffer 
	///(e.g. more than 1000x1000px with 32bit color).
    void size(const Vec2ui& size);

protected:
	WaylandAppContext* appContext_ = nullptr;
    unsigned int shmSize_ = 1024 * 1024 * 5; //5MB

    Vec2ui size_;
    wl_buffer* buffer_;
    wl_shm_pool* pool_;
	std::uint8_t* data_;
	unsigned int format_; //argb > bgra > rgba > abgr > xrgb (all 32 bits)

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
	ServerCallback(wl_callback& callback);
	nytl::Callback<void(wl_callback&, unsigned int)> onCallback;

	//called by the wayland listener
    void done(wl_callback&, unsigned int data);
};

///Holds information about a wayland output.
class Output
{
public:
    Output(wl_output& outp);
    ~Output();

    wl_output* wlOutput = nullptr;

    Vec2i position;
    Vec2i size;
    Vec2i physicalSize;

    int subpixel;
    int refreshRate;

    unsigned int flags;

    std::string make;
    std::string model;

    int transform;
    int scale;
};

}//wayland

//convert function
MouseButton linuxToButton(unsigned int id);
Key linuxToKey(unsigned int id);

std::string cursorToWayland(CursorType type);
CursorType waylandToCursor(std::string id);

}
