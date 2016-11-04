#pragma once

#include <ny/include.hpp>

namespace ny
{

///Interface for classes that are able to handle ny::Event objects.
///Can be used as synchronization and polymorphic dispatch method.
class EventHandler
{
public:
    EventHandler() = default;
    virtual ~EventHandler() = default;

	///Handle the given event.
	///\return true when the event was processed, false otherwise
    virtual bool handleEvent(const Event&) { return false; }
};

}
