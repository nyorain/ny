// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <nytl/nonCopyable.hpp> // nytl::NonCopyable

#include <functional> // std::function
#include <mutex> // std::atomic

// header-only implementation.
// can be used independently in other projects

namespace ny {

/// Abstract base interface for LoopControl implementations.
/// Implemented by functions running loops that can be stopped or for which can additional
/// functions be queued even from other threads.
/// Should be designed threadsafe.
/// \sa LoopControl
class LoopInterface {
public:
	/// Returns a dummy LoopInterface objet.
	/// Needed to implement LoopControl lockfree but threadsafe, since this object will
	/// be set as dummy LoopControl::impl_, so calling member functions will not result
	/// in a memory error.
	static LoopInterface& dummy() { static LoopInterface instance; return instance; }

public:
	/// Should stop the associated loop. Return false if stopping it failed somehow.
	/// Must be threadsafe and wake up the thread if currently blocking/waiting.
	/// Must also work if called from within the loop (e.g. callback).
	virtual bool stop() { return false; }

	/// Should call the given function in the next loop iteration from the loop thread.
	/// Must be implemented threadsafe, and work if called from within the loop.
	virtual bool call(std::function<void()>) { return false; }

protected:
	inline LoopInterface(LoopControl& lc);
	inline virtual ~LoopInterface();

private:
	LoopInterface() = default;
	LoopControl* control_ {};
};


/// Can be used to control loops.
/// Since this object is passed to a loop function and implemented by it, it
/// can be used to control the loop from the same thread (i.e. callback functions called
/// from within the loop) or even from another thread.
/// Designed threadsafe, i.e. calling any of its function in any thread will never result
/// in undefined behaviour (requiring a threadsafe LoopInterface implementation).
/// If the loop is currently waiting/blocking for events calling member functions of the
/// associated LoopControl object will wake up the loop.
class LoopControl : public nytl::NonMovable {
public:
	LoopControl() = default;
	~LoopControl() = default;

	/// Stops the loop, i.e. asks it to return as soon as possible.
	/// Returns false if this loop object is invalid or stopping it failed somehow.
	inline bool stop();

	/// Asks the loop to run the given function from within.
	/// Especially useful for synchronization. The function is not guaranteed to be called
	/// as sonn as possible, but as soon as the next loop iteration is done.
	/// Functions are guaranteed to be called in the order they are queued here.
	/// Returns false if this LoopControl object is invalid or queueing the function failed.
	/// Will not wait for the function to be executed or finish.
	inline bool call(std::function<void()> func);

	/// Returns whether this object is valied, i.e. whether it member functions will
	/// have any effect.
	/// If this returns false all member functions are guaranteed to return false.
	/// Note that this cannot really be used in any meaningful context since the
	/// LoopControl might be reset from another thread.
	bool valid() const { return &impl() != &LoopInterface::dummy(); }

protected:
	friend class LoopInterface;
	inline void impl(LoopInterface& impl);
	inline LoopInterface& impl() const;

protected:
	std::reference_wrapper<LoopInterface> impl_ {LoopInterface::dummy()};
	mutable std::mutex mutex_; // use shared mutex?
};

// - inline implementation -
LoopInterface::LoopInterface(LoopControl& lc) : control_(&lc)
{
	control_->impl(*this);
}

LoopInterface::~LoopInterface()
{
	if(control_)
		control_->impl(LoopInterface::dummy());
}

void LoopControl::impl(LoopInterface& li)
{
	std::lock_guard<std::mutex> lock(mutex_);
	impl_ = li;
}

LoopInterface& LoopControl::impl() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return impl_.get();
}

bool LoopControl::stop()
{
	std::lock_guard<std::mutex> lock(mutex_);
	return impl_.get().stop();
}

bool LoopControl::call(std::function<void()> func)
{
	std::lock_guard<std::mutex> lock(mutex_);
	return impl_.get().call(std::move(func));
}

/// NOTE: a few implementation detilas on LoopControl
/// It is required to synchronize it with a mutex (an atomic impl pointer is not enough)
/// since it has to be assured that the impl cannot be changed (usually reset)
/// while an impl function is called.
/// Must/should be even threadsafe in the following case:
/// - the loop has ended, i.e. resets the LoopControl impl
/// - another thread calls stop/call
/// - the implementation might be destroyed while another thread is still calling
///	  a member function
/// - the mutex guards an impl change as well as the impl functions so it can be
/// assured that this scenario works.
/// The cost of this might be a blocking LoopControl::impl and therefore a
/// blocking LoopInterface destructor. But since the impl functions that might
/// be waited on should not block themselves it should not be a problem

} // namespace ny
