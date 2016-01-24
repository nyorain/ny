#include <ny/app/eventDispatcher.hpp>
#include <ny/app/eventHandler.hpp>

#include <nytl/log.hpp>

namespace ny
{

EventDispatcher::EventDispatcher()
{
	eventDispatcher_ = std::thread(&EventDispatcher::dispatcherThreadFunc, this);
}

EventDispatcher::~EventDispatcher()
{
	exit();
}

void EventDispatcher::exit()
{
	exit_.store(1);
	eventCV_.notify_one();

	if(eventDispatcher_.joinable()) eventDispatcher_.join();
}

void EventDispatcher::sendEvent(Event& event)
{
	auto it = callbacks_.find(event.type());
	if(it != callbacks_.cend())
	{
		it->second(event);
	}

	if(event.handler) event.handler->handleEvent(event);
	else nytl::sendWarning("EventDispatcher: Got event with no handler, type ", event.type());
}

void EventDispatcher::dispatcherThreadFunc()
{
    std::unique_lock<std::mutex> lck(eventMtx_);
    while(1)
    {
        while(events_.empty() && !exit_.load()) 
		{
			eventCV_.wait(lck);
		}
        if(exit_.load())
		{
			nytl::sendLog("EventDispatcher: exiting thread");
			return;
		}

        auto ev = std::move(events_.front());
        events_.pop_front();

        lck.unlock();
        sendEvent(*ev);
		lck.lock();
    }
}

void EventDispatcher::dispatch(EventPtr&& event)
{
    if(!event.get())
    {
		nytl::sendWarning("EventDispatcher::dispatch: invalid event");
        return;
    }

	//sendEvent(*event);
	//return;

    { 
		std::lock_guard<std::mutex> lck(eventMtx_);
        if(event->overrideable() && 0)
        {
            for(auto& stored : events_)
            {
                if(stored->type() == event->type())
                {
                    stored = std::move(event);
					break;
                }
            }
        }

		if(event) events_.emplace_back(std::move(event));
    } 

    eventCV_.notify_one();
}

void EventDispatcher::dispatch(const Event& event)
{
	dispatch(clone(event));
}

}
