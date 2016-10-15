#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/base/event.hpp>

#include <nytl/vec.hpp>
#include <nytl/callback.hpp>
#include <nytl/functionTraits.hpp>

#include <type_traits>

namespace ny
{

//wayland specific events
namespace eventType
{
	namespace wayland
	{
		constexpr auto frame = 1001u;
		constexpr auto configure = 1002u;
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
class FrameEvent : public EventBase<eventType::wayland::frame, FrameEvent>
{
public:
	using EvBase::EvBase;
	wl_callback* wlCallback;
};

///This Event will be sent to a WaylandWindowContext if it holds a surface with shell
///(wayland or xdg) role that is configured. Holds new size and resized edge.
class ConfigureEvent : public EventBase<eventType::wayland::configure, ConfigureEvent>
{
public:
	using EvBase::EvBase;
	ny::WindowEdge edge;
	nytl::Vec2ui size;
};

//TODO: use one shared shm_pool (or maybe one per thread or sth.)
///Defines a wayland shm buffer that can be resized.
class ShmBuffer
{
public:
	ShmBuffer() = default;
    ShmBuffer(WaylandAppContext& ac, Vec2ui size, unsigned int stride = 0);
    ~ShmBuffer();

	ShmBuffer(ShmBuffer&& other);
	ShmBuffer& operator=(ShmBuffer&& other);

    Vec2ui size() const { return size_; }
    unsigned int dataSize() const { return stride_ * size_.y; }
	unsigned int format() const { return format_; }
	unsigned int stride() const { return stride_; }
	std::uint8_t& data(){ return *data_; }
    wl_buffer& wlBuffer() const { return *buffer_; }

    void use() { used_ = true; }
    bool used() const { return used_; }

	///Will trigger a buffer recreate if the given size exceeds the current size.
	///At the beginning 5MB will be allocated for a buffer 
	///(e.g. more than 1000x1000px with 32bit color).
	///\return true if the data pointer changed, false if it stayed the same, i.e.
	///returns whether a new shm pool had to be created
    bool size(const Vec2ui& size, unsigned int stride = 0);

protected:
	WaylandAppContext* appContext_ {};

    Vec2ui size_;
	unsigned int stride_ {};
    unsigned int shmSize_ = 1024 * 1024 * 5; //5MB at the beginning

    wl_buffer* buffer_ {};
    wl_shm_pool* pool_ {};
	std::uint8_t* data_ {};
	unsigned int format_ {}; //wayland format; argb > bgra > rgba > abgr > xrgb (all 32 bits)
    bool used_ {0}; //whether the compositor owns the buffer atm

protected:
	//could be also named recreate. Does automatically call destroy
    void create();

	//frees all resources. Called by destructor and (re)create
    void destroy();

    //release cb
    friend void bufferRelease(void*, wl_buffer*);
    void wasReleased(){ used_ = false; }
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
	struct Mode
	{
		Vec2ui size;
		unsigned int flags;
		unsigned int refresh;
	};

public:
    Output(WaylandAppContext& ac, wl_output& outp, unsigned int id);
    ~Output();

public:
	WaylandAppContext* appContext;
    wl_output* wlOutput = nullptr;
	unsigned int globalID;

    Vec2i position;
    Vec2ui physicalSize;
	std::vector<Mode> modes;

    unsigned int subpixel;
    unsigned int refreshRate;

    std::string make;
    std::string model;

    unsigned int transform;
    int scale;
};

namespace detail
{
	template<typename F, F f, 
		typename R = typename nytl::FunctionTraits<F>::ReturnType,
		typename ArgsTuple = typename nytl::FunctionTraits<F>::ArgTuple>
	struct MemberCallback;

	template<typename F, F f, typename R, typename... Args>
	struct MemberCallback<F, f, R, std::tuple<Args...>>
	{
		using Class = typename nytl::FunctionTraits<F>::Class;
		static auto call(void* self, Args&&... args)
		{
			return (static_cast<Class*>(self)->*f)(std::forward<Args>(args)...);
		}
	};

	template<typename F, F f, typename... Args>
	struct MemberCallback<F, f, void, std::tuple<Args...>> 
	{
		using Class = typename nytl::FunctionTraits<F>::Class;
		static void call(void* self, Args&&... args) 
		{
			(static_cast<Class*>(self)->*f)(std::forward<Args>(args)...);
		}
	};
}

//Can be used to implement wayland callbacks directly to member functions.
//Does only work if the first parameter of the callback is a void* userdata pointer and
//the pointer holds the object of which the given member function should be called.
template<typename F, F f> 
auto constexpr memberCallback = &detail::MemberCallback<std::decay_t<F>, f>::call;

//C++17
// template<auto f> 
// constexpr auto memberCallback = &detail::MemberCallback<std::decay_t<decltype(f)>, f>::call;

}//wayland

//convert function
WindowEdge waylandToEdge(unsigned int edge);
unsigned int edgeToWayland(WindowEdge edge);

ImageDataFormat waylandToImageFormat(unsigned int format);
unsigned int imageFormatToWayland(ImageDataFormat format);

}
