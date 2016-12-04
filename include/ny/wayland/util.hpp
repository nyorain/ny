// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/wayland/include.hpp>
#include <ny/windowListener.hpp>

#include <nytl/vec.hpp>
#include <nytl/callback.hpp>
#include <nytl/functionTraits.hpp>
#include <nytl/compFunc.hpp>

#include <type_traits>

namespace ny
{

namespace wayland
{

//TODO: use one shared shm_pool for all ShmBuffers?
//At least use pool of pools but not one for every buffer. This is really bad

///Wraps and manages a wayland shm buffer.
class ShmBuffer
{
public:
	ShmBuffer() = default;
    ShmBuffer(WaylandAppContext& ac, nytl::Vec2ui size, unsigned int stride = 0);
    ~ShmBuffer();

	ShmBuffer(ShmBuffer&& other);
	ShmBuffer& operator=(ShmBuffer&& other);

    nytl::Vec2ui size() const { return size_; }
    unsigned int dataSize() const { return stride_ * size_.y; }
	unsigned int format() const { return format_; }
	unsigned int stride() const { return stride_; }
	std::uint8_t& data(){ return *data_; }
    wl_buffer& wlBuffer() const { return *buffer_; }

	///Sets the internal used flag to true. Should be called everytime the buffer
	///is attached to a surface. The ShmBuffer will automatically clear the flag
	///when the compositor send the buffer release event.
	///\sa used
    void use() { used_ = true; }

	///Returns whether the ShmBuffer is currently in use by the compostior.
	///If this is true, the data should not be accessed in any way.
	///\sa use
    bool used() const { return used_; }

	///Will trigger a buffer recreate if the given size exceeds the current size.
	///At the beginning 5MB will be allocated for a buffer
	///(e.g. more than 1000x1000px with 32bit color).
	///\return true if the data pointer changed, false if it stayed the same, i.e.
	///returns whether a new shm pool had to be created
    bool size(const nytl::Vec2ui& size, unsigned int stride = 0);

protected:
	WaylandAppContext* appContext_ {};

    nytl::Vec2ui size_;
	unsigned int stride_ {};
    unsigned int shmSize_ = 1024 * 1024 * 5; //5MB at the beginning

    wl_buffer* buffer_ {};
    wl_shm_pool* pool_ {};
	uint8_t* data_ {};
	unsigned int format_ {}; //wayland format; argb > bgra > rgba > abgr > xrgb (all 32 bits)
    bool used_ {0}; //whether the compositor owns the buffer atm

protected:
    void create(); //(re)creates the buffer for the set size/format/stride. Calls destroy
    void destroy(); //frees all associated resources
    void released(){ used_ = false; } //registered as listener function
};

///Holds information about a wayland output.
class Output
{
public:
	///Represents a single possible output mode.
	///Can be set when a WindowContext is made fullscreen.
	///Usually outputs have multiple output modes.
	struct Mode
	{
		nytl::Vec2ui size;
		unsigned int flags;
		unsigned int refresh;
	};

	///All received output information.
	///Note that this Information is not finished yet, if the done member is false.
	struct Information
	{
		nytl::Vec2i position;
		nytl::Vec2ui physicalSize;
		std::vector<Mode> modes;
		unsigned int subpixel {};
		unsigned int refreshRate {};
		std::string make;
		std::string model;
		unsigned int transform {};
		int scale {};
		bool done {};
	};

public:
	Output() = default;
    Output(WaylandAppContext& ac, wl_output& outp, unsigned int id);
    ~Output();

	Output(Output&& other) noexcept;
	Output& operator=(Output&& other) noexcept;

	WaylandAppContext& appContext() const { return *appContext_; }
	wl_output& wlOutput() const { return *wlOutput_; }
	unsigned int name() const { return globalID_; }
	const Information& information() const { return information_; }

	bool valid() const { return wlOutput_ && appContext_; }

protected:
	WaylandAppContext* appContext_;
    wl_output* wlOutput_ {};
	unsigned int globalID_ {};
	Information information_ {};

protected:
	void geometry(int, int, int, int, int, const char*, const char*, int);
	void mode(unsigned int, int, int, int);
	void done();
	void scale(int);
};

///Utility template that allows to associate a numerical value (name) with wayland globals.
template<typename T>
struct NamedGlobal
{
	T* global = nullptr;
	unsigned int name = 0;

	operator T*() const { return global; }
};

}//wayland

//The following could also be moved to some general util file since many c libraries use
//the user-data-void*-as-first-callback-parameter idiom and this make connecting this
//to OO-C++ code really convinient.

namespace detail
{
	template<typename F, F f,
		typename Signature = typename nytl::FunctionTraits<F>::Signaure,
		bool Last = false,
		typename R = typename nytl::FunctionTraits<Signature>::ReturnType,
		typename ArgsTuple = typename nytl::FunctionTraits<Signature>::ArgTuple>
	struct MemberCallback;

	template<typename F, F f, typename Signature, typename R, typename... Args>
	struct MemberCallback<F, f, Signature, false, R, std::tuple<Args...>>
	{
		using Class = typename nytl::FunctionTraits<F>::Class;
		static auto call(void* self, Args... args)
		{
			using Func = nytl::CompatibleFunction<Signature>;
			auto func = Func(nytl::memberCallback(f, static_cast<Class*>(self)));
			return func(std::forward<Args>(args)...);
		}
	};

	template<typename F, F f, typename Signature, typename... Args>
	struct MemberCallback<F, f, Signature, false, void, std::tuple<Args...>>
	{
		using Class = typename nytl::FunctionTraits<F>::Class;
		static void call(void* self, Args... args)
		{
			using Func = nytl::CompatibleFunction<Signature>;
			auto func = Func(nytl::memberCallback(f, static_cast<Class*>(self)));
			func(std::forward<Args>(args)...);
		}
	};

	template<typename F, F f, typename Signature, typename R, typename... Args>
	struct MemberCallback<F, f, Signature, true, R, std::tuple<Args...>>
	{
		using Class = typename nytl::FunctionTraits<F>::Class;
		static auto call(Args... args, void* self)
		{
			using Func = nytl::CompatibleFunction<Signature>;
			auto func = Func(nytl::memberCallback(f, static_cast<Class*>(self)));
			return func(std::forward<Args>(args)...);
		}
	};

	template<typename F, F f, typename Signature, typename... Args>
	struct MemberCallback<F, f, Signature, true, void, std::tuple<Args...>>
	{
		using Class = typename nytl::FunctionTraits<F>::Class;
		static void call(Args... args, void* self)
		{
			using Func = nytl::CompatibleFunction<Signature>;
			auto func = Func(nytl::memberCallback(f, static_cast<Class*>(self)));
			func(std::forward<Args>(args)...);
		}
	};
} //detail

///Can be used to implement wayland callbacks directly to member functions.
///Does only work if the first parameter of the callback is a void* userdata pointer and
///the pointer holds the object of which the given member function should be called.
///Note that while the same could (usually) also be achieved by just casting the
///member function pointer to a non-member function pointer taking a void* pointer,
///this way is C++ standard conform and should also not have any call real overhead.
///\tparam F The type of the member callback function. This template parameter
///can be avoided by using <auto f> in C++17.
///\tparam f The member callback function.
///\tparam S The original callback signature. Since the actually passed callback function
///can take less parameters than the wayland listener function takes (nytl::CompatibleFunction is
///used to achieve parameter mapping at compile time) this parameter should be the
///Signature the raw listener function (i.e. the function returnd by this expression) should
///have.
///|tparam L Whether the object void pointer is the last parameter instead of the first.
template<typename F, F f, typename S = typename nytl::FunctionTraits<F>::Signature, bool L = false>
auto constexpr memberCallback = &detail::MemberCallback<std::decay_t<F>, f, S, L>::call;

//C++17
// template<auto f>
// constexpr auto memberCallback = &detail::MemberCallback<std::decay_t<decltype(f)>, f>::call;

///TODO: does not belong here... rather some common util file
///Utility template that allows a generic list of connectable objects.
///Used by WaylandAppContext for its fd listeners.
template<typename T>
class ConnectionList : public nytl::Connectable
{
public:
	class Value : public T
	{
	public:
		using T::T;
		nytl::ConnectionID clID_;
	};

	std::vector<Value> items;
	nytl::ConnectionID highestID;

public:
	bool disconnect(nytl::ConnectionID id) override
	{
		for(auto it = items.begin(); it != items.end(); ++it)
		{
			if(it->clID_ == id)
			{
				items.erase(it);
				return true;
			}
		}

		return false;
	}

	nytl::Connection add(const T& value)
	{
		items.emplace_back();
		static_cast<T&>(items.back()) = value;
		items.back().clID_ = nextID();
		return {*this, items.back().clID_};
	}

	nytl::ConnectionID nextID()
	{
		++reinterpret_cast<std::uintptr_t&>(highestID);
		return highestID;
	}
};

///Used for e.g. move/resize requests where the serial of the trigger can be given
///All wayland event callbacks that retrieve a serial value should create a WaylandEventData
///object and pass it to the event handler.
class WaylandEventData : public ny::EventData
{
public:
    WaylandEventData(unsigned int xserial) : serial(xserial) {};
    unsigned int serial;
};

///Wayland std::error_category implementation for one wayland interface.
///Only used for wayland protocol errors, for other errors wayland uses posix
///error codes, so generic_category will be used.
///Note that there has to be one ErrorCategory for every wayland interface since
///otherwise errors cannot be correctly represented just using an integer value
///(i.e. the error code).
class WaylandErrorCategory : public std::error_category
{
public:
	WaylandErrorCategory(const wl_interface&);
	~WaylandErrorCategory() = default;

	const char* name() const noexcept override { return name_.c_str(); }
	std::string message(int code) const override;

	const wl_interface& interface() const { return interface_; }

protected:
	const wl_interface& interface_;
	std::string name_;
};

///Converts the given wl_shell or xdg_shell surface edge enumerations value to the corresponding
///WindowEdge value.
///Undefined behaviour for invalid edge values.
WindowEdge waylandToEdge(unsigned int edge);

///Converts a WindowEdge enumeration value to the corresponding wl_shell
///(or xdg_shell, they are the same) surface edge enumeration value.
///Undefined behaviour for invalid edge values.
unsigned int edgeToWayland(WindowEdge edge);

///Converts the given wayland shm format to the corresponding ImageDataFormat enum value.
///Returns ImageDataFormat::none for invalid or not representable formats.
ImageDataFormat waylandToImageFormat(unsigned int format);

///Converts the given ImageDataFormat enum value to the corresponding wayland shm format.
///Returns -1 for invalid or not representable formats.
int imageFormatToWayland(ImageDataFormat format);

}
