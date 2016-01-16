#pragma once

#include <ny/app/include.hpp>
#include <nytl/make_unique.hpp>
#include <nytl/cloneable.hpp>

#include <type_traits>
#include <stdexcept>

namespace ny
{

//eventType
//custom events should put their type id in this namespace
namespace eventType
{
constexpr unsigned int destroy = 1;
constexpr unsigned int reparent = 2;
}

//eventData, used by backends
class EventData : public cloneable<EventData>
{
public:
    virtual ~EventData() = default; //for dynamic cast
};

//event//////////////////
class Event : public abstractCloneable<Event>
{
public:
    Event(EventHandler* h = nullptr, EventData* d = nullptr) : handler(h), data(d) {};
    virtual ~Event() = default;

    Event(const Event& other) : handler(other.handler), data(nullptr) 
		{ if(other.data.get()) data = nytl::clone(other.data); }

    Event& operator=(const Event& other)
    {
        handler = other.handler;
        if(other.data.get()) data = nytl::clone(other.data);
        else data.reset();
        return *this;
    }

    Event(Event&& other) noexcept = default;
    Event& operator=(Event&& other) noexcept = default;

    EventHandler* handler {nullptr};
    std::unique_ptr<EventData> data {nullptr}; 

    virtual unsigned int type() const = 0;
    virtual bool overrideable() const { return 0; }
};

using EventPtr = std::unique_ptr<Event>;

//eventBase
template<typename T, unsigned int Type, bool Override = 0>
class EventBase : public deriveCloneable<Event, T>
{
public:
	using EvBase = EventBase;
	using CloenableEvBase = deriveCloneable<Event, T>;

public:
	using CloenableEvBase::CloenableEvBase;

    //event
    virtual unsigned int type() const override final { return Type; }
    virtual bool overrideable() const override final { return Override; }
};

//destroy
class DestroyEvent : public EventBase<DestroyEvent, eventType::destroy, 1>
{
public:
	using EvBase::EvBase;
};

//destroy
class ReparentEvent : public EventBase<ReparentEvent, eventType::reparent, 1>
{
public:
    ReparentEvent(EventHandler* handler = nullptr, EventHandlerNode* newparent = nullptr, 
			EventData* d = nullptr) : EvBase(handler, d), newParent(newparent) {};

    EventHandlerNode* newParent {nullptr};
};

}

