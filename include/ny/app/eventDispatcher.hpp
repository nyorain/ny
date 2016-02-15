#pragma once

#include <ny/include.hpp>
#include <ny/base/event.hpp>
#include <nytl/callback.hpp>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <deque>
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
};

///\brief Threadsafe event dispatcher implementation. 
///It is safe to call dispatch() from multiple threads because this functions just pushes
///the event to the thread-safe event queue. Only a call to dispatchEvents() really sends all
///events.
class ThreadedEventDispatcher : public EventDispatcher
{
public:

protected:
    std::deque<EventPtr> events_;
    std::mutex eventMtx_;
    std::condition_variable eventCV_;

public:
	ThreadedEventDispatcher();
	~ThreadedEventDispatcher();

	virtual void dispatch(EventPtr&& event) override;
	virtual void dispatch(const Event& event) override;
	virtual void dispatch(Event&& event) override;

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
};

}
