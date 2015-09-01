#include <ny/eventHandler.hpp>

#include <ny/app.hpp>
#include <ny/error.hpp>
#include <ny/event.hpp>

#include <iostream>

namespace ny
{

eventHandler::eventHandler() : hierachyBase()
{
}

eventHandler::eventHandler(eventHandler& parent) : hierachyBase(parent)
{
}

bool eventHandler::processEvent(const event& event)
{
    if(event.type() == eventType::destroy)
    {
        destroy();
        return true;
    }

    return false;
}


}
