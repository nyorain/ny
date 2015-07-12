#pragma once

#include <ny/include.hpp>

#include <ny/util/nonCopyable.hpp>
#include <ny/util/thread.hpp>

#include <vector>
#include <mutex>
#include <thread>

namespace ny
{

class eventHandler : public nonCopyable, public threadSafeObj
{
protected:
    std::vector<eventHandler*> children_;
    eventHandler* parent_;

    virtual void create(eventHandler& parent);
    virtual void reparent(eventHandler& newParent);

    eventHandler();

public:
    eventHandler(eventHandler& parent);
    virtual ~eventHandler();

    virtual void destroy();

    virtual bool processEvent(event& event); //return value defines if base class already cared about the object

    virtual eventHandler* getParent() const { return parent_; };
    virtual std::vector<eventHandler*> getChildren() const { return children_; }

    virtual void addChild(eventHandler& child);
    virtual void removeChild(eventHandler& child);

    virtual bool isValid() const { return (parent_); }
};

}
