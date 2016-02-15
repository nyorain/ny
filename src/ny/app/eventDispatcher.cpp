#include <ny/app/eventDispatcher.hpp>
#include <ny/base/eventHandler.hpp>
#include <ny/base/loopControl.hpp>

#include <ny/base/log.hpp>

namespace ny
{

struct DispatcherControlImpl : public LoopControlImpl
{
	std::atomic<bool>* stop_;
	std::condition_variable* cv_;

	DispatcherControlImpl(std::atomic<bool>& stop, std::condition_variable& cv)
		: stop_(&stop), cv_(&cv) {}
	virtual void stop() override { if(!stop_) return; stop_->store(1); cv_->notify_one(); }
};

//Default
void EventDispatcher::sendEvent(const Event& event)
{
	auto it = onEvent.find(event.type());
	if(it != onEvent.cend())
	{
		it->second(event);
	}

	if(event.handler) event.handler->handleEvent(event);
	else noEventHandler(event);
}

void EventDispatcher::noEventHandler(const Event& event) const
{
	sendWarning("ny::EventDispatcher: Received Event with no handler of type ", event.type());
}

//Threaded
ThreadedEventDispatcher::ThreadedEventDispatcher()
{
}

ThreadedEventDispatcher::~ThreadedEventDispatcher()
{
}

void ThreadedEventDispatcher::dispatchEvents()
{
    std::unique_lock<std::mutex> lck(eventMtx_);
	while(!events_.empty())
	{
        auto ev = std::move(events_.front());
        events_.pop_front();

        lck.unlock();
        sendEvent(*ev);
		lck.lock();
	}
}

void ThreadedEventDispatcher::dispatchLoop(LoopControl& control)
{
	std::atomic<bool> stop {0};
	control.impl_ = std::make_unique<DispatcherControlImpl>(&stop, &eventCV_);
    std::unique_lock<std::mutex> lck(eventMtx_);

    while(1)
    {
        while(events_.empty() && !stop.load()) 
		{
			eventCV_.wait(lck);
		}
        if(stop.load())
		{
			return;
		}

        auto ev = std::move(events_.front());
        events_.pop_front();

        lck.unlock();
        sendEvent(*ev);
		lck.lock();
    }
}

void ThreadedEventDispatcher::dispatch(EventPtr&& event)
{
    if(!event.get())
    {
		sendWarning("EventDispatcher::dispatch: invalid event");
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

void ThreadedEventDispatcher::dispatch(const Event& event)
{
	dispatch(clone(event));
}

void ThreadedEventDispatcher::dispatch(Event&& event)
{
	dispatch(cloneMove(std::move(event)));
}

}
