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

///\brief Abstract EventDispatcher interface.
class EventDispatcher
{
protected:
	virtual void sendEvent(const Event& event) = 0;

public:
	EventDispatcher() = default;
	virtual ~EventDispatcher() = default;

	virtual void dispatch(EventPtr&& event){ sendEvent(*event); };
	virtual void dispatch(const Event& event){ sendEvent(event); }
	virtual void dispatch(Event&& event){ sendEvent(event); }
};

///\brief Default EventDispatcher implementation which sends the events to the specified handlers.
class DefaultEventDispatcher : public EventDispatcher
{
};

///\brief Threadsafe event dispatcher class.
class ThreadedEventDispatcher
{
protected:
    std::thread eventDispatcher_;
    std::deque<EventPtr> events_;
    std::mutex eventMtx_;
    std::condition_variable eventCV_;
	std::atomic<bool> exit_ {0};

	std::map<unsigned int, Callback<void(Event&)>> Callbacks_;

protected:
	void dispatcherThreadFunc();
	void sendEvent(Event& event);

public:
	EventDispatcher();
	~EventDispatcher();

	void exit();
	void dispatch(EventPtr&& event);
	void dispatch(const Event& event);

	template<typename F> 
	Connection onEvent(unsigned int type, F&& func){ return Callbacks_[type].add(func); }
};

}
