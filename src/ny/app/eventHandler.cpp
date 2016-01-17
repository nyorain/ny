#include <ny/app/eventHandler.hpp>
#include <ny/app/event.hpp>

#include <nytl/log.hpp>

#include <iostream>

namespace ny
{

//node
EventHandlerNode::EventHandlerNode() : EventHandler(), hierachyBase()
{
}

EventHandlerNode::EventHandlerNode(EventHandlerNode& parent) : EventHandler(), hierachyBase(parent)
{
}

void EventHandlerNode::create(EventHandlerNode& parent)
{
    hierachyBase::create(parent);
}

bool EventHandlerNode::processEvent(const Event& event)
{
	if(!processEventImpl(event)) 
	{
		if(!parent())
		{
			nytl::sendWarning("EventHandlerNode::processEvent: no parent");
			return false;
		}

		if(event.passable())
		{
			return parent()->processEvent(event);
		}
	}

	return true;
}

bool EventHandlerNode::processEventImpl(const Event& event)
{
    if(event.type() == eventType::destroy)
    {
        destroy();
        return true;
    }
    else if(event.type() == eventType::reparent)
    {
        auto& ev = static_cast<const ReparentEvent&>(event);

        if(ev.newParent) reparent(*ev.newParent);
        else nytl::sendWarning("eventHandlerNode::processEvent: reparentEvent has invalid parent.");

        return true;
    }

    return false;
}

//root
bool EventHandlerRoot::processEventImpl(const Event& event)
{
	if(event.type() == eventType::destroy)
	{
		destroy();
		return true;
	}

	return false;
}

bool EventHandlerRoot::processEvent(const Event& event)
{
	return processEventImpl(event);
}

}
