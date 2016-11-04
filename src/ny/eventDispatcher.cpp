#include <ny/eventDispatcher.hpp>
#include <ny/event.hpp>
#include <ny/eventHandler.hpp>
#include <ny/loopControl.hpp>
#include <ny/log.hpp>

#include <nytl/scope.hpp>

namespace ny
{

namespace
{

//LoopControlImpl for ThreadedEvetnDispatchers dispatch loop.
struct DispatcherControlImpl : public LoopControlImpl
{
	std::atomic<bool>* stop_;
	std::condition_variable* cv_;

	DispatcherControlImpl(std::atomic<bool>& stop, std::condition_variable& cv)
		: stop_(&stop), cv_(&cv) {}
	virtual void stop() override { if(!stop_) return; stop_->store(1); cv_->notify_one(); }
};

}

//EventDispatcher
void EventDispatcher::send(Event& event) 
{
	auto it = onEvent.find(event.type());
	if(it != onEvent.cend()) it->second(*this, event);

	it = onEvent.find(0);
	if(it != onEvent.cend()) it->second(*this, event);

	if(event.handler) event.handler->handleEvent(event);
	else warning("ny::EventDispatcher: Received Event with no handler of type ", event.type());
}

void EventDispatcher::processEvents()
{
    std::unique_lock<std::mutex> lck(eventMtx_);
	while(!events_.empty())
	{
        auto ev = std::move(events_.front());
        events_.pop_front();

        lck.unlock();
        send(*ev);
		lck.lock();
	}
}

void EventDispatcher::processLoop(LoopControl& control)
{
	std::atomic<bool> stop {0};
	control.impl_ = std::make_unique<DispatcherControlImpl>(stop, eventCV_);
	auto loopguard = nytl::makeScopeGuard([&]{ control.impl_.reset(); });
    std::unique_lock<std::mutex> lck(eventMtx_);

    while(1)
    {
		if(events_.empty())
		{
			//Just signal all stored promises since there cannot be any that are associated with
			//queued events, since there are none.
			for(auto& promise : promises_) promise.second.set_value();
			promises_.clear();

	        while(events_.empty() && !stop.load())
			{
				eventCV_.wait(lck);
			}
		}

        if(stop.load())
		{
			break;
		}

        auto ev = std::move(events_.front());
        events_.pop_front();

        lck.unlock();
        send(*ev);
		lck.lock();

		//signal all promises waiting for the processed event
		for(auto it = promises_.begin(); it < promises_.end();)
		{
			if(it->first == ev.get())
			{
				it->second.set_value();
				it = promises_.erase(it);
			}
			else
			{
				++it;
			}
		}
    }
}

void EventDispatcher::dispatch(std::unique_ptr<Event>&& event)
{
    if(!event.get())
    {
		warning("EventDispatcher::dispatch: invalid event");
        return;
    }

	if(!event->handler)
	{
		warning("ny::EventDispatcher: Received Event with no handler of type ", event->type());
		return;
	}

	onDispatch(*this, *event);

    {
		std::lock_guard<std::mutex> lck(eventMtx_);
        if(event->overrideable())
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

void EventDispatcher::dispatch(Event&& event)
{
	dispatch(cloneMove(event));
}

void EventDispatcher::dispatchSync(EventPtr&& event)
{
	dispatch(std::move(event));
	auto fut = sync(); //XXX: may acually call sync after some additional events where queued.
	fut.wait();
}

void EventDispatcher::dispatchSync(Event&& event)
{
	dispatchSync(cloneMove(event));
}

std::future<void> EventDispatcher::sync()
{
	std::lock_guard<std::mutex> lck(eventMtx_);
	if(events_.empty())
	{
		std::promise<void> prom;
		prom.set_value();
		return prom.get_future();
	}

	promises_.emplace_back();
	promises_.back().first = events_.back().get();
	return promises_.back().second.get_future();
}

std::future<void> EventDispatcher::waitIdle()
{
	std::lock_guard<std::mutex> lck(eventMtx_);
	if(events_.empty())
	{
		std::promise<void> prom;
		prom.set_value();
		return prom.get_future();
	}

	promises_.emplace_back();
	promises_.back().first = nullptr;
	return promises_.back().second.get_future();
}

std::size_t EventDispatcher::eventCount() const
{
	std::lock_guard<std::mutex> lck(eventMtx_);
	return events_.size();
}

}
