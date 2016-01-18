#pragma once

#include <ny/include.hpp>
#include <ny/app/event.hpp>
#include <nytl/callback.hpp>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <deque>
#include <map>
#include <atomic>

namespace ny
{

///\brief Threadsafe event dispatcher class.
class EventDispatcher
{
protected:
    std::thread eventDispatcher_;
    std::deque<EventPtr> events_;
    std::mutex eventMtx_;
    std::condition_variable eventCV_;
	std::atomic<bool> exit_ {0};

	std::map<unsigned int, callback<void(Event&)>> callbacks_;

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
	connection onEvent(unsigned int type, F&& func){ return callbacks_[type].add(func); }
};

}
