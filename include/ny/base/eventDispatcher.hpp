#pragma once

#include <ny/include.hpp>
#include <ny/base/event.hpp>
#include <nytl/callback.hpp>
#include <nytl/nonCopyable.hpp>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <deque>
#include <future>
#include <map>
#include <atomic>

namespace ny
{

///\brief Async Threadsafe EventDispatcher.
///It is safe to call dispatch() from multiple threads because this functions just pushes
///the event to the thread-safe event queue. Only a call to processEvents() really sends all
///events.
class ThreadedEventDispatcher : public nytl::NonCopyable
{
public:
	///Callback that will be called everytime before an event is queued for dispatching.
	///Note that the registered function might be called by mulitple threads (even at the
	///same time).
	Callback<void(ThreadedEventDispatcher&, const Event&)> onDispatch;

public:
	ThreadedEventDispatcher();
	~ThreadedEventDispatcher();

	///\{
	///Queues the given event for processing.
	void dispatch(std::unique_ptr<Event>&& event);
	void dispatch(Event&& event);
	///\}

	///\{
	///Queues the given event for dispatch and blocks the calling thread until it has
	///been dispatched.
	///\warning This function can easily result in a deadlock if there is no other thread
	///processing messages.
	void dispatchSync(std::unique_ptr<Event>&& event);
	void dispatchSync(Event&& event);
	///\}

	///Returns a future that will be signaled when all events queued at the moment this
	///function is called are processed.
	///If there are no events at the time of calling this function the future will instantly
	///be signaled.
	///If the eventDispatcher is destructed before all currently queued events could be dispatched
	///the future will implicitly be set to std::broken_promise.
	///\warning Waiting for the future can easily result in a deadlock if there is no other thread
	///processing messages.
	///\sa waitIdle
	virtual std::future<void> sync();

	///Returns a future that will be signaled when there are no more events in the queue to be
	///dispatched.
	///If there are no events at the time of calling this function the future will instantly
	///be signaled.
	///If the eventDispatcher is destructed before all currently queued events could be dispatched
	///the future will implicitly be set to std::broken_promise.
	///\warning Waiting for the future can easily result in a deadlock if there is no other thread
	///processing messages.
	///\sa sync
	virtual std::future<void> waitIdle();

	///Proccess all currently queued events and returns after there are no events left.
	///\sa processLooop
	virtual void processEvents();

	///Dispatches all incoming events until ThreadEventDispatcher::exit() is called.
	///This function runs in the calling thread and does not return until the LoopControl
	///is signaled to stop.
	///This must therefore be done from inside the event processing system or from a
	///different thread.
	///\sa processEvents
	virtual void processLoop(LoopControl& control);

	///\{
	///Returns the condition variable that will be notified every time a event is received.
	///This condition variable will addtionally notified when ThreadedEventDispatcher::exit()
	///is called.
	const std::condition_variable& cv() const { return eventCV_; }
	std::condition_variable& cv() { return eventCV_; }
	///\}

	///Returns the count of events currently waiting for processing.
	///Lock that this function locks the internal mutex and might therefore should rather not
	///be called if it can be avoided.
	std::size_t eventCount() const;

protected:
    std::deque<std::unique_ptr<Event>> events_;
    std::condition_variable eventCV_;
    mutable std::mutex eventMtx_;

	std::vector<std::pair<Event*, std::promise<void>>> promises_;
};

}
