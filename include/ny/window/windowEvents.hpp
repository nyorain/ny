#pragma once

#include <ny/window/include.hpp>
#include <ny/app/event.hpp>

#include <nytl/vec.hpp>

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
constexpr unsigned int context = 20;

constexpr unsigned int contextCreate = 1; //contextEventType
}

class FocusEvent : public EventBase<FocusEvent, eventType::windowFocus>
{
public:
    FocusEvent(EventHandler* h = nullptr, bool gain = 0, EventData* d = nullptr)
        : EvBase(h, d), focusGained(gain) {}

    bool focusGained;
};

class SizeEvent : public EventBase<SizeEvent, eventType::windowSize>
{
public:
    SizeEvent(EventHandler* h = nullptr, vec2ui s = vec2ui(), bool ch = 1, EventData* d = nullptr)
        : EvBase(h, d), size(s), change(ch) {}

    vec2ui size;
    bool change;
};

class ShowEvent : public EventBase<ShowEvent, eventType::windowShow>
{
public:
    ShowEvent(EventHandler* h = nullptr, bool ch = 1, EventData* d = nullptr)
        : EvBase(h, d), change(ch) {}

    //showState here
    bool change;
};

class PositionEvent : public EventBase<PositionEvent, eventType::windowPosition, 1>
{
public:
    PositionEvent(EventHandler* h = nullptr, vec2i pos = vec2i(), bool ch = 1, EventData* d = nullptr)
        : EvBase(h, d), position(pos), change(ch) {}

    vec2i position {0, 0};
    bool change;
};

class DrawEvent : public EventBase<DrawEvent, eventType::windowDraw, 1>
{
public:
    DrawEvent(EventHandler* h = nullptr, EventData* d = nullptr) : EvBase(h, d) {}
};

//better sth like "using refreshEvent = eventBase<eventType::windowRefresh>;"? since it has no additional members
class RefreshEvent : public EventBase<RefreshEvent, eventType::windowRefresh, 1>
{
public:
    RefreshEvent(EventHandler* h = nullptr, EventData* d = nullptr) : EvBase(h, d) {}
};

class ContextEvent : public Event //own ContextEvents could be derived form this
{
public:
    ContextEvent(EventHandler* h = nullptr, EventData* d = nullptr) : Event(h, d) {}

    virtual unsigned int type() const override final { return eventType::context; }
    virtual unsigned int contextType() const = 0;
};

class ContextCreateEvent : public deriveCloneable<ContextEvent, ContextCreateEvent>
{
public:
    ContextCreateEvent(EventHandler* h = nullptr, EventData* d = nullptr) 
		: deriveCloneable<ContextEvent, ContextCreateEvent>(h, d) {}

    virtual unsigned int contextType() const override { return eventType::contextCreate; }
};

}
