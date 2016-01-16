#include <ny/app/eventHandler.hpp>
#include <ny/app/event.hpp>

#include <nytl/log.hpp>

#include <iostream>

namespace ny
{

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


}
