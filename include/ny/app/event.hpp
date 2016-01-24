#pragma once

#include <ny/include.hpp>
#include <nytl/make_unique.hpp>
#include <nytl/cloneable.hpp>

#include <type_traits>
#include <stdexcept>

namespace ny
{

//EventData, used by backends
class EventData : public cloneable<EventData> {};

//Event
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
template<unsigned int Type, typename T = void, bool Override = 0>
class EventBase : public deriveCloneable<Event, T>
{
public:
	using EvBase = EventBase;
	using typename deriveCloneable<Event, T>::cloneableBase;

public:
	using cloneableBase::cloneableBase;

    //event
    virtual unsigned int type() const override final { return Type; }
    virtual bool overrideable() const override final { return Override; }
};

//void specialization
template<unsigned int Type, bool Override>
class EventBase<Type, void, Override> 
	: public deriveCloneable<Event, EventBase<Type, void, Override>>
{
public:
	using EvBase = EventBase;
	using typename deriveCloneable<Event, EvBase>::cloneableBase;

public:
	using cloneableBase::cloneableBase;

    //event
    virtual unsigned int type() const override final { return Type; }
    virtual bool overrideable() const override final { return Override; }
};

}

