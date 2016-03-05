#pragma once

#include <ny/include.hpp>

namespace ny
{

///Interface for classes that are able to handle ny::Event objects.
class EventHandler
{
public:
    EventHandler() = default;
    virtual ~EventHandler() = default;

	///\return true when the event was processed, false otherwise
    virtual bool handleEvent(const Event&) = 0; 
};

}
