#pragma once

#include <ny/include.hpp>
#include <nyutil/hierachy.hpp>

#include <memory>

namespace ny
{

class eventHandler : public hierachyNode<eventHandler>
{
protected:
    using hierachyBase = hierachyNode<eventHandler>;

    eventHandler();
public:
    eventHandler(eventHandler& parent);
    virtual ~eventHandler() = default;

    virtual bool processEvent(std::unique_ptr<event> event); //returns if event was processed (1) or ignored (0)
};

}
