// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/wayland/include.hpp>
#include <ny/windowListener.hpp>
#include <ny/image.hpp>

#include <nytl/vec.hpp>
#include <nytl/callback.hpp>
#include <nytl/functionTraits.hpp>

#include <type_traits>
#include <vector>
#include <string>

namespace ny {
namespace wayland {

// TODO: shmbuffer in preferred format, don't just always use argb
// TODO: use one shared shm_pool for all ShmBuffers?
// At least use pool of pools but not one for every buffer. This is really bad

/// Wraps and manages a wayland shm buffer.
class ShmBuffer {
public:
	ShmBuffer() = default;
	ShmBuffer(WaylandAppContext& ac, nytl::Vec2ui size, unsigned int stride = 0);
	~ShmBuffer();

	ShmBuffer(ShmBuffer&& other);
	ShmBuffer& operator=(ShmBuffer&& other);

	nytl::Vec2ui size() const { return size_; }
	unsigned int dataSize() const { return stride_ * size_[1]; }
	unsigned int format() const { return format_; }
	unsigned int stride() const { return stride_; }
	std::byte& data(){ return *data_; }
	wl_buffer& wlBuffer() const { return *buffer_; }

	/// Sets the internal used flag to true. Should be called everytime the buffer
	/// is attached to a surface. The ShmBuffer will automatically clear the flag
	/// when the compositor send the buffer release event.
	/// \sa used
	void use() { used_ = true; }

	/// Returns whether the ShmBuffer is currently in use by the compostior.
	/// If this is true, the data should not be accessed in any way.
	/// \sa use
	bool used() const { return used_; }

	/// Will trigger a buffer recreate if the given size exceeds the current size.
	/// At the beginning 5MB will be allocated for a buffer
	/// (e.g. more than 1000x1000px with 32bit color).
	/// \return true if the data pointer changed, false if it stayed the same, i.e.
	/// returns whether a new shm pool had to be created
	bool size(nytl::Vec2ui size, unsigned int stride = 0);

protected:
	WaylandAppContext* appContext_ {};

	nytl::Vec2ui size_;
	unsigned int stride_ {};
	unsigned int shmSize_ = 1024 * 1024 * 5; // 5MB at the beginning

	wl_buffer* buffer_ {};
	wl_shm_pool* pool_ {};
	std::byte* data_ {};
	unsigned int format_ {}; // wayland format; argb > bgra > rgba > abgr > xrgb (all 32 bits)
	bool used_ {0}; // whether the compositor owns the buffer atm

protected:
	void create(); // (re)creates the buffer for the set size/format/stride. Calls destroy
	void destroy(); // frees all associated resources
	void released(wl_buffer*) { used_ = false; } // registered as listener function
};

/// Utility template that allows to associate a numerical value (name) with wayland globals.
/// \tparam T The type of the wayland global, e.g. wl_output
template<typename T>
struct NamedGlobal {
	T* global = nullptr;
	unsigned int name = 0;

	operator T*() const { return global; }
};

/// Returns the name of the given protocol error.
/// Returns "<unknown interface error>" if it is not known.
const char* errorName(const wl_interface& interface, int error);

} // namespace wayland

// The following could also be moved to some general util file since many c libraries use
// the user-data-void*-as-first-callback-parameter idiom and this make connecting this
// to OO-C++ code really convinient.

namespace detail {

template<typename F, F f, typename Signature, bool Last>
struct MemberCallback;

template<typename F, F f, typename R, typename... Args>
struct MemberCallback<F, f, R(Args...), false> {
	using Class = typename nytl::FunctionTraits<F>::Class;
	static auto call(void* self, Args... args) {
		return (static_cast<Class*>(self)->*f)(std::forward<Args>(args)...);
	}
};

template<typename F, F f, typename R, typename... Args>
struct MemberCallback<F, f, R(Args...), true> {
	using Class = typename nytl::FunctionTraits<F>::Class;
	static auto call(Args... args, void* self) {
		return (static_cast<Class*>(self)->*f)(std::forward<Args>(args)...);
	}
};

} // namespace detail

// TODO: does not belong here... rather some common util file
/// Can be used to implement wayland callbacks directly to member functions.
/// Does only work if the first parameter of the callback is a void* userdata pointer and
/// the pointer holds the object of which the given member function should be called.
/// Note that while the same could (usually) also be achieved by just casting the
/// member function pointer to a non-member function pointer taking a void* pointer,
/// this way is C++ standard conform and should also not have any call real overhead.
/// \tparam f The member callback function.
/// \tparam S The original callback signature. Allows small differences to the
/// signature of f.
/// |tparam L Whether the object void pointer is the last parameter instead of the first.
template<auto f, typename S = typename nytl::FunctionTraits<decltype(f)>::Signature, bool L = false>
constexpr auto memberCallback = &detail::MemberCallback<std::decay_t<decltype(f)>, f, S, L>::call;

// TODO: does not belong here... rather some common util file
/// Utility template that allows a generic list of connectable objects.
/// Used by WaylandAppContext for its fd listeners.
template<typename T>
class ConnectionList : public nytl::Connectable {
public:
	struct Value : public T {
		using T::T;
		nytl::ConnectionID clID_;
	};

	std::vector<Value> items;
	nytl::ConnectionID highestID;

public:
	bool disconnect(const nytl::ConnectionID& id) override {
		for(auto it = items.begin(); it != items.end(); ++it) {
			if(it->clID_.get() == id.get()) {
				items.erase(it);
				return true;
			}
		}

		return false;
	}

	nytl::Connection add(const T& value) {
		items.emplace_back();
		static_cast<T&>(items.back()) = value;
		items.back().clID_ = {nextID()};
		return {*this, items.back().clID_};
	}

	nytl::ConnectionID nextID() {
		++highestID.value;
		return highestID;
	}
};

///Used for e.g. move/resize requests where the serial of the trigger can be given
///All wayland event callbacks that retrieve a serial value should create a WaylandEventData
///object and pass it to the event handler.
class WaylandEventData : public ny::EventData {
public:
	WaylandEventData(unsigned int xserial) : serial(xserial) {};
	unsigned int serial;
};

/// Converts the given wl_shell_surface edge enumerations value to the corresponding
/// WindowEdge value.
/// Undefined behaviour for invalid edge values.
WindowEdge waylandToEdge(unsigned int edge);

/// Converts a WindowEdge enumeration value to the corresponding wl_shell_surface
/// edge enumeration value.
/// Undefined behaviour for invalid edge values.
unsigned int edgeToWayland(WindowEdge edge);

/// Converts the given wayland shm format to the corresponding ImageFormat enum value.
/// Returns imageFormats::none for invalid or not representable formats.
ImageFormat waylandToImageFormat(unsigned int wlShmFormat);

/// Converts the given ImageDataFormat enum value to the corresponding wayland shm format.
/// Returns -1 for invalid or not representable formats.
int imageFormatToWayland(const ImageFormat&);

/// Converts the given xdg shell protocol edge to a ToplevelState enum value.
/// Returns ToplevelState::unknown for none/invalid states.
/// Works for xdg shell v6 and stable.
ToplevelState waylandToState(unsigned int wlState);

/// Converts the given ToplevelState enum value to a xdg shell protoocl state value.
/// Returns 0 for values that cannot be represented.
/// Works for xdg shell v6 and stable.
unsigned int stateToWayland(ToplevelState);

/// Converts the given wayland dnd action to DndAction.
DndAction waylandToDndAction(unsigned int wlAction);
nytl::Flags<DndAction> waylandToDndActions(unsigned int wlActions);

/// Converts the given dnd action to its corresponding value in the wayland
/// core protocol.
unsigned int dndActionToWayland(DndAction);
unsigned int dndActionsToWayland(nytl::Flags<DndAction>);

} // namespace ny
