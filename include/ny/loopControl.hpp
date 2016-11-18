// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <functional>
#include <atomic>

//This header and its functionality can be used without linking to ny.

namespace ny
{

///Abstract base interface for LoopControl implementations.
///Implemented by functions running loops that can be stopped or for which can additional
///functions be queued even from other threads.
///Note that loops should usually not implement this interface directly, but rather use
///the LoopInterfaceGuard base class which does not implement any of the pure virtual
///functions but takes care of correct settings/resetting of the associated LoopControl object.
///\sa LoopControl
class LoopInterface
{
public:
	///Returns a dummy LoopInterface objet.
	///Needed to implement LoopControl lockfree but threadsafe, since this object will
	///be set as dummy LoopControl::impl_, so calling member functions will not result
	///in a memory error.
	static LoopInterface& dummy() { static LoopInterface instance; return instance; }

public:
	virtual ~LoopInterface() = default;

	///Should stop the associated loop. Return false if stopping it failed somehow.
	///Must be threadsafe and wake up the thread if currently blocking/waiting.
	///Must also work if called from within the loop (e.g. callback).
	virtual bool stop() { return false; }

	///Should call the given function in the next loop iteration from the loop thread.
	///Must be implemented threadsafe, and work if called from within the loop.
	virtual bool call(std::function<void()> func) { return false; }

protected:
	static inline void set(LoopControl& lc, LoopInterface& impl);
};


///Can be used to control loops.
///Since this object is passed to a loop function and implemented by it, it
///can be used to control the loop from the same thread (i.e. callback functions called
///from within the loop) or from another thread.
///It is threadsafe, i.e. calling any of its function in any thread will never result
///in undefined behaviour (required that the loop function implements it correctly).
///Only lifetime synchronization has to managed by the application.
///If the loop is currently waiting/blocking for events calling member functions of the
///associated LoopControl object will wake up the loop.
class LoopControl
{
public:
	LoopControl() = default;
	~LoopControl() { stop(); }

	///Stops the loop, i.e. returns as soon as possible.
	///Returns false if this loop object is invalid or stopping it failed somehow.
	bool stop() { return impl_.load()->stop(); }

	///Asks the loop to run the given function from within.
	///Especially useful for synchronization. The function is not guaranteed to be called
	///as sonn as possible, but as soon as the next loop iteration is done.
	///Functions are guaranteed to be called in the order they are queued here.
	///Returns false if this LoopControl object is invalid or queueing the function failed.
	bool call(std::function<void()> func) { return impl_.load()->call(std::move(func)); }

	///Returns whether this object is valied, i.e. whether it member functions can
	///be be called. If this returns false all member functions are guaranteed to return false.
	bool valid() const { return impl_.load() && impl_.load() != &LoopInterface::dummy(); }
	operator bool() const { return valid(); }

protected:
	friend class LoopInterface;
	std::atomic<LoopInterface*> impl_ {&LoopInterface::dummy()};
};


////Manages correct assocating/resetting of a LoopInterface implementation with a Loop
///object. Should be preferred over implementing the raw LoopInterface object since
///it assures that the Loop object will not be put in an state that results in undefined
///behaviour. Should be constructed with the loopControl object the LoopInterface is
///created for.
class LoopInterfaceGuard : public LoopInterface
{
public:
	LoopInterfaceGuard(LoopControl& lc) : lc_(lc) { set(lc_, *this); }
	~LoopInterfaceGuard() { set(lc_, LoopInterface::dummy()); }

protected:
	LoopControl& lc_;
};

//inline implementation
void LoopInterface::set(LoopControl& loopControl, LoopInterface& impl)
{
	loopControl.impl_ = &impl;
}

}
