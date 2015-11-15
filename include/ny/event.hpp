#pragma once

#include <ny/include.hpp>
#include <nytl/make_unique.hpp>

#include <type_traits>
#include <stdexcept>

namespace ny
{

//eventType
//custom events should put their type id in this namespace
namespace eventType
{
constexpr unsigned int invalid = 0;
constexpr unsigned int destroy = 1;
constexpr unsigned int reparent = 2;
}

//eventData, used by backends
class eventData
{
public:
    virtual ~eventData() = default; //for dynamic cast
    virtual std::unique_ptr<eventData> clone() const = 0;
};

template <typename T>
class eventDataBase : public eventData
{
public:
    virtual std::unique_ptr<eventData> clone() const override
        { return make_unique<T>(static_cast<const T&>(*this)); }
};

//event//////////////////
class event
{
public:
    event(eventHandler* h = nullptr, eventData* d = nullptr) : handler(h), data(d) {};
    virtual ~event() = default;

    event(const event& other) : handler(other.handler), data(nullptr) { if(other.data.get()) data = other.data->clone(); }
    event& operator=(const event& other)
    {
        handler = other.handler;
        if(other.data.get()) data = other.data->clone();
        else data.reset();
        return *this;
    }

    event(event&& other) : handler(other.handler), data(std::move(other.data)) {}
    event& operator=(event&& other) { handler = other.handler; data = std::move(other.data); return *this; }

    eventHandler* handler {nullptr};
    std::unique_ptr<eventData> data {nullptr}; //place for the backend to transport data (e.g. serial numbers for custom resize / move)

    //type
    virtual std::unique_ptr<event> clone() const = 0;
    virtual unsigned int type() const = 0;
    virtual bool overrideable() const { return 0; }
};

using eventPtr = std::unique_ptr<event>;

//eventBase
template<typename T, unsigned int Type, bool Override = 0>
class eventBase : public event
{
protected:
    using evBase = eventBase<T, Type, Override>;

public:
    eventBase(eventHandler* xhandler = nullptr, eventData* xdata = nullptr) : event(xhandler, xdata){};

    //event
    virtual std::unique_ptr<event> clone() const override { return make_unique<T>(static_cast<const T&>(*this)); }
    virtual unsigned int type() const override final { return Type; }
    virtual bool overrideable() const override final { return Override; }
};

//destroy
class destroyEvent : public eventBase<destroyEvent, eventType::destroy, 1>
{
public:
    destroyEvent(eventHandler* h = nullptr, eventData* d = nullptr) : evBase(h, d) {};
};

//destroy
class reparentEvent : public eventBase<reparentEvent, eventType::reparent, 1>
{
public:
    reparentEvent(eventHandler* h = nullptr, eventHandler* nh = nullptr, eventData* d = nullptr) : evBase(h, d), newParent(nh) {};
    eventHandler* newParent {nullptr};
};

//event cast
template<typename T>
std::unique_ptr<T> event_cast(std::unique_ptr<event>&& ptr)
{
    static_assert(std::is_base_of<event, T>::value, "you can only cast into a derived event class");

    auto ret = dynamic_cast<T*>(ptr.release());
    if(ptr) return std::unique_ptr<T>(ret);

    throw std::logic_error("event_cast<unique_ptr>: tried to cast into wrong type");
}

template<typename T>
const T& event_cast(const event& ev)
{
    static_assert(std::is_base_of<event, T>::value, "you can only cast into a derived event class");

    auto ret = dynamic_cast<const T*>(&ev);
    if(ret) return *ret;

    throw std::logic_error("event_cast<const ref>: tried to cast into wrong type");
}

}

