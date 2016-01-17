#pragma once

#include <ny/include.hpp>

namespace ny
{

//eventHandler base
class EventHandler
{
public:
    EventHandler() = default;
    virtual ~EventHandler() = default;

	//returns if event was processed (1) or not handled (0)
    virtual bool processEvent(const Event&) { return 0; }; 
};

}
