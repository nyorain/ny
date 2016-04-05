#pragma once

#include <ny/include.hpp>
#include <ny/base/event.hpp>

#include <nytl/vec.hpp>
#include <nytl/clone.hpp>

#include <memory>

namespace ny
{

namespace eventType
{
	constexpr unsigned int windowSize = 11;
	constexpr unsigned int windowPosition = 12;
	constexpr unsigned int windowDraw = 13;
	constexpr unsigned int windowShow = 14;
	constexpr unsigned int windowFocus = 15;
	constexpr unsigned int windowRefresh = 16;
	constexpr unsigned int windowClose = 17;
	constexpr unsigned int windowContext = 20;

	namespace context
	{
		constexpr unsigned int create = 1; //contextEventType
	}
}

//
class FocusEvent : public EventBase<eventType::windowFocus, FocusEvent>
{
public:
	using EvBase::EvBase;
    bool gained {0};
};

//
class SizeEvent : public EventBase<eventType::windowSize, SizeEvent>
{
public:
	using EvBase::EvBase;

    Vec2ui size {0, 0};
    bool change = 0;
};

class ShowEvent : public EventBase<eventType::windowShow, ShowEvent>
{
public:
	using EvBase::EvBase;

    bool show = 0;
    //showState here
};

class CloseEvent : public EventBase<eventType::windowClose, CloseEvent>
{
public:
	using EvBase::EvBase;
};

class PositionEvent : public EventBase<eventType::windowPosition, PositionEvent, 1>
{
public:
	using EvBase::EvBase;

    Vec2i position {0, 0};
    bool change = 0;
};

class DrawEvent : public EventBase<eventType::windowDraw, DrawEvent, 1>
{
public:
	using EvBase::EvBase;
};

class RefreshEvent : public EventBase<eventType::windowRefresh, RefreshEvent, 1>
{
public:
	using EvBase::EvBase;
};

//ContextEvent
class ContextEvent : public Event //own ContextEvents could be derived form this
{
public:
    ContextEvent(EventHandler* h = nullptr, EventData* d = nullptr) : Event(h, d) {}

    virtual unsigned int type() const override final { return eventType::windowContext; }
    virtual unsigned int contextType() const = 0;
};

template<unsigned int ContextType, typename T, bool Override = 0>
class ContextEventBase : public DeriveCloneable<ContextEvent, T>
{
public:
	using ContextEvBase = ContextEventBase;
	using typename DeriveCloneable<ContextEvent, T>::CloneableBase;

public:
	using CloneableBase::CloneableBase;

	virtual bool overrideable() const override { return Override; }
	virtual unsigned int contextType() const override { return ContextType; }
};

class ContextCreateEvent : public DeriveCloneable<ContextEvent, ContextCreateEvent>
{
public:
	using CloneableBase::CloneableBase;
    virtual unsigned int contextType() const override { return eventType::context::create; }
};

}
