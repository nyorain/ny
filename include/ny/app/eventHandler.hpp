#pragma once

#include <ny/app/include.hpp>
#include <nytl/hierachy.hpp>

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

//eventHandlerHierachyNode
class EventHandlerNode : public EventHandler, public hierachyNode<EventHandlerNode>
{
protected:
    using hierachyBase = hierachyNode<EventHandlerNode>;

    EventHandlerNode();
    void create(EventHandlerNode& parent);

public:
    EventHandlerNode(EventHandlerNode& parent);
    virtual ~EventHandlerNode() = default;

    virtual bool processEvent(const Event& ev) override;
};

//eventHandlerRoot
using EventHandlerRoot = hierachyRoot<EventHandlerNode>;

}
