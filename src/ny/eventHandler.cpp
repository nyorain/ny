#include <ny/eventHandler.hpp>

#include <ny/app.hpp>
#include <ny/error.hpp>
#include <ny/event.hpp>

#include <iostream>

namespace ny
{

eventHandlerNode::eventHandlerNode() : eventHandler(), hierachyBase()
{
}

eventHandlerNode::eventHandlerNode(eventHandlerNode& parent) : eventHandler(), hierachyBase(parent)
{
}

void eventHandlerNode::create(eventHandlerNode& parent)
{
    hierachyBase::create(parent);
}

bool eventHandlerNode::processEvent(const event& event)
{
    if(event.type() == eventType::destroy)
    {
        destroy();
        return true;
    }
    else if(event.type() == eventType::reparent)
    {
        auto ev = event_cast<reparentEvent>(event);
        auto parent = dynamic_cast<eventHandlerNode*>(ev.newParent);

        if(parent) reparent(*parent);
        else nyWarning("eventHandlerNode::processEvent: received reparentEvent without valid parent.");

        return true;
    }

    return false;
}


}
