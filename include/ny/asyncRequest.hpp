// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <nytl/nonCopyable.hpp> // nytl::NonMovable
#include <nytl/scope.hpp> // nytl::ScopeGuard

#include <ny/appContext.hpp> // ny::AppContext
#include <ny/loopControl.hpp> // ny::LoopControl
#include <functional> // std::function

namespace ny {

/// Allows to deal with single-threaded asynchronous requests.
/// Usually wait() will not just wait but instead keep the internal event loop running.
/// It also allows to register a callback to be called on request completion.
/// It can also easily be used for synchronous requests, since it could be ready from the
/// beginning. Since several operations are implemented sync or async by different backends,
/// this abstraction is needed.
/// The registered callback function will always be called from the gui thread during wait
/// or some other event dispatching functions.
/// The member functions of AsyncRequest objects should always only be used from the main gui
/// thread that it was retrieved from. It cannot be directly used from other threads like
/// e.g. std::future), but the flexible callback design can be easily used to achieve something
/// similiar.
template <typename R>
class AsyncRequest {
public:
	virtual ~AsyncRequest() = default;

	/// Waits until the request is finished.
	/// Returns false if an error occurred while waiting, i.e. if the associated AppContext
	/// became invalid.
	/// If this call returns true, the registered callback function was triggered or (if there
	/// is none) this request will be ready and the return object can be retrieved with get.
	/// Note that therefore the Request is not guaranteed to be ready or valid after
	/// this call completes, since the registered callback might have already received its
	/// return object and the return value must always be checked.
	/// While waiting, the internal gui thread event loop will be run.
	/// Calls to wait on the same AsyncRequest object should not be nested.
	virtual bool wait(LoopControl* lc = nullptr) = 0;

	/// Returns whether the AsyncRequest is valid.
	/// If this is false, calling other member functions results in undefined behaviour.
	/// Requests which return objects where retrieved (by get or callback) are invalid.
	virtual bool valid() const = 0;

	/// Returns whether the AsyncRequest is ready, i.e. if an object of type R can
	/// be retrieved with get. Calling get if this returns false results in an
	/// exception.
	virtual bool ready() const = 0;

	/// Returns the retrieved object if it is available.
	/// Otherwise (i.e. if this function was called while ready() returns false) this
	/// will throw a std::logic_error.
	virtual R get() = 0;

	/// Sets the callback that is triggered on completion.
	/// Note that there is already a callback function for this request, it is
	/// cleared and set to the given one. To reset/clear the current callback just call this
	/// with an empty (defualt-constructed) function.
	/// If this is called by the request is ready, the callback will be instanly triggered.
	virtual void callback(std::function<void(AsyncRequest&)>) = 0;
};

/// Default AsyncRequest implementation that behaves as specified and just waits to
/// be completed by the associated AppContext.
template <typename R>
class DefaultAsyncRequest : public AsyncRequest<R>, public nytl::NonMovable {
public:
	DefaultAsyncRequest(AppContext& ac) : appContext_(&ac) {}
	DefaultAsyncRequest(R value) : ready_(true), value_(std::move(value)) {}

	bool wait(LoopControl* lc = nullptr) override
	{
		if(ready_) return true;
		LoopControl localControl;
		if(!lc) lc = &localControl;
		topControl_ = lc;
		auto controlGuard = nytl::makeScopeGuard([&]{ topControl_ = {}; });

		return appContext_->dispatchLoop(*lc);
	}

	void callback(std::function<void(AsyncRequest<R>&)> func) override
	{
		if(ready_) func(*this);
		callback_ = func;
	}

	R get() override { ready_ = false; appContext_ = {}; return std::move(value_); }
	bool ready() const override { return ready_; }
	bool valid() const override { return (appContext_) || (ready_); }

	/// This function has to be called by the AppContext event dispatching system
	/// when the request completes.
	/// It will store the passed value, end a potential wait call and trigger the registered
	/// callback function (if any).
	void complete(R value)
	{
		ready_ = true;
		value_ = std::move(value);
		if(callback_) callback_(*this);
		if(topControl_) topControl_->stop();
	}

protected:
	AppContext* appContext_ {};
	std::function<void(AsyncRequest<R>&)> callback_;
	bool ready_ {};
	LoopControl* topControl_ {};
	R value_;
};

}
