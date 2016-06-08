#pragma once

#include <ny/include.hpp>
#include <ny/base/event.hpp>
#include <nytl/callback.hpp>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <deque>
#include <future>
#include <map>
#include <atomic>

namespace ny
{

///\brief EventDispatcher base class.
///Instantly sends the events to their EventHandler and outputs a warning if it receives an event
///without specified handler. Is able to call additionally callbacks when dispatching events.
class EventDispatcher
{
protected:
	virtual void sendEvent(const Event& event);
	virtual void noEventHandler(const Event& event) const;

public:
	std::map<unsigned int, Callback<void(const Event&)>> onEvent;

public:
	EventDispatcher() = default;
	virtual ~EventDispatcher() = default;

	virtual void dispatch(EventPtr&& event){ sendEvent(*event); };
	virtual void dispatch(const Event& event){ sendEvent(event); }
	virtual void dispatch(Event&& event){ sendEvent(event); }

	virtual void dispatchSync(EventPtr&& event) { sendEvent(*event); }
	virtual void dispatchSync(const Event& event) { sendEvent(event); }
	virtual void dispatchSync(Event&& event) { sendEvent(event); }
};

///\brief Threadsafe event dispatcher implementation.
///It is safe to call dispatch() from multiple threads because this functions just pushes
///the event to the thread-safe event queue. Only a call to dispatchEvents() really sends all
///events.
class ThreadedEventDispatcher : public EventDispatcher
{
protected:
    std::deque<EventPtr> events_;
    std::condition_variable eventCV_;
    mutable std::mutex eventMtx_;

	std::vector<std::pair<Event*, std::promise<void>>> promises_;

public:
	ThreadedEventDispatcher();
	~ThreadedEventDispatcher();

	virtual void dispatch(EventPtr&& event) override;
	virtual void dispatch(const Event& event) override;
	virtual void dispatch(Event&& event) override;

	///\{
	///Queues the given event for dispatch and blocks the calling thread until it has
	///been dispatched.
	virtual void dispatchSync(EventPtr&& event) override;
	virtual void dispatchSync(const Event& event) override;
	virtual void dispatchSync(Event&& event) override;
	///\}

	///Returns a future that will be signaled when all events queued at the moment this
	///function is called are processed.
	///If there are no events at the time of calling this function the future will instantly
	///be signaled.
	///If the eventDispatcher is destructed before all currently queued events could be dispatched
	///the future will implicitly be set to std::broken_promise.
	///\sa waitIdle
	virtual std::future<void> sync();

	///Returns a future that will be signaled when there are no more events in the queue to be
	///dispatched.
	///If there are no events at the time of calling this function the future will instantly
	///be signaled.
	///If the eventDispatcher is destructed before all currently queued events could be dispatched
	///the future will implicitly be set to std::broken_promise.
	virtual std::future<void> waitIdle();

	///Dispatches all currently queued events.
	virtual void dispatchEvents();

	///Dispatches all incoming events until ThreadEventDispatcher::exit() is called.
	///This function runs in the calling thread and does not return until exit() is called.
	///Therefore exit() must be called from inside the event dispatching system or from a
	///different thread.
	virtual void dispatchLoop(LoopControl& control);

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
};

}
