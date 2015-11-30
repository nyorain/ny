#pragma once

#include <ny/include.hpp>
#include <nytl/hierachy.hpp>

namespace ny
{

//eventHandler base
class eventHandler
{
public:
    eventHandler() = default;
    virtual ~eventHandler() = default;

    virtual bool processEvent(const event& ev) { return 0; }; //returns if event was processed (1) or ignored (0)
};

//eventHandlerHierachyNode
class eventHandlerNode : public eventHandler, public hierachyNode<eventHandlerNode>
{
protected:
    using hierachyBase = hierachyNode<eventHandlerNode>;

    eventHandlerNode();
    void create(eventHandlerNode& parent);

public:
    eventHandlerNode(eventHandlerNode& parent);
    virtual ~eventHandlerNode() = default;

    virtual bool processEvent(const event& ev);
};

}
