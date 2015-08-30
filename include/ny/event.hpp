#pragma once

#include <ny/include.hpp>
#include <memory>

namespace ny
{

//eventType
//custom events should put their type id in this namespace
namespace eventType
{
constexpr unsigned int invalid = 0;
constexpr unsigned int destroy = 1;
}

//eventData, used by backends
class eventData
{
public:
    virtual ~eventData(){}; //for dynamic cast
};

//event//////////////////
class event
{
public:
    event(eventHandler* xhandler = nullptr, eventData* xdata = nullptr) : handler(xhandler), data(xdata) {};
    virtual ~event() = default;

    eventHandler* handler {nullptr};
    std::unique_ptr<eventData> data {nullptr}; //place for the backend to transport data (e.g. serial numbers for custom resize / move), todo: unique ptr

    //cast
    template<class T> T& to() { return static_cast<T&>(*this); };
    template<class T> const T& to() const { return static_cast<const T&>(*this); };

    //type
    virtual unsigned int type() const = 0;
};

//eventBase
template<unsigned int Type> class eventBase : public event
{
protected:
    using evBase = eventBase<Type>;

    eventBase(eventHandler* xhandler = nullptr, eventData* xdata = nullptr) : event(xhandler, xdata){};
    virtual unsigned int type() const override { return Type; }
};

//destroy
class destroyEvent : public eventBase<eventType::destroy>
{
public:
    destroyEvent(eventHandler* h = nullptr, eventData* d = nullptr) : evBase(h, d) {};

};


}

