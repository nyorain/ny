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
public:
	///Map of Callbacks for different event types that will be called everytime an event
	///with the given type is sent.
	///All functions registered for eventType 0 will be called for every event.
	std::map<unsigned int, Callback<void(EventDispatcher&, Event&)>> onEvent;

public:
	EventDispatcher() = default;
	virtual ~EventDispatcher() = default;

	///\{
	///Dispatch the given event and returns immediatly.
	///Depending on the Dispatcher implementation it may or may not be changed/queued/sent later.
	///The default implementation just directly sends the event from the calling thread.
	///These functions may move from the given parameters.
	virtual void dispatch(std::unique_ptr<Event>&& event){ send(*event); };
	virtual void dispatch(Event&& event){ send(event); }
	///\}

	///\{
	///Dispatch the given event to its handler and waits till it has been processed.
	///The default implementation just directly sends the event from the calling thread.
	///The functions may move from the given paramters.
	virtual void dispatchSync(std::unique_ptr<Event>&& event) { send(*event); }
	virtual void dispatchSync(Event&& event) { send(event); }
	///\}

	///Just sends the given event to its handler and triggers the callback function.
	virtual void send(Event& event);

protected:
	///Action to be performed when there is an event without a handler.
	///The default implementation just outputs a warning and discards the event.
	virtual void noEventHandler(Event& event) const;
};

///\brief Async Threadsafe event dispatcher implementation.
///It is safe to call dispatch() from multiple threads because this functions just pushes
///the event to the thread-safe event queue. Only a call to processEvents() really sends all
///events.
///Therefore this EventDispatcher implementation can be considered asynchronous.
class ThreadedEventDispatcher : public EventDispatcher
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
	virtual void dispatch(std::unique_ptr<Event>&& event) override;
	virtual void dispatch(Event&& event) override;
	///\}

	///\{
	///Queues the given event for dispatch and blocks the calling thread until it has
	///been dispatched.
	///\warning This function can easily result in a deadlock if there is no other thread
	///processing messages.
	virtual void dispatchSync(std::unique_ptr<Event>&& event) override;
	virtual void dispatchSync(Event&& event) override;
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
