#pragma once

#include <ny/include.hpp>
#include <ny/event.hpp>

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
}

class focusEvent : public eventBase<focusEvent, eventType::windowFocus>
{
public:
    focusEvent(eventHandler* h = nullptr, bool gain = 0, eventData* d = nullptr)
        : evBase(h, d), focusGained(gain) {}

    bool focusGained;
};

class sizeEvent : public eventBase<sizeEvent, eventType::windowSize>
{
public:
    sizeEvent(eventHandler* h = nullptr, vec2ui s = vec2ui(), bool ch = 1, eventData* d = nullptr)
        : evBase(h, d), size(s), change(ch) {}

    vec2ui size;
    bool change;
};

class showEvent : public eventBase<showEvent, eventType::windowShow>
{
public:
    showEvent(eventHandler* h = nullptr, bool ch = 1, eventData* d = nullptr)
        : evBase(h, d), change(ch) {}

    //showState here
    bool change;
};

class positionEvent : public eventBase<positionEvent, eventType::windowPosition, 1>
{
public:
    positionEvent(eventHandler* h = nullptr, vec2i pos = vec2i(), bool ch = 1, eventData* d = nullptr)
        : evBase(h, d), position(pos), change(ch) {}

    vec2i position {0, 0};
    bool change;
};

class drawEvent : public eventBase<drawEvent, eventType::windowDraw, 1>
{
public:
    drawEvent(eventHandler* h = nullptr, eventData* d = nullptr) : evBase(h, d) {}
};

//better sth like "using refreshEvent = eventBase<eventType::windowRefresh>;"? since it has no additional members
class refreshEvent : public eventBase<refreshEvent, eventType::windowRefresh, 1>
{
public:
    refreshEvent(eventHandler* h = nullptr, eventData* d = nullptr) : evBase(h, d) {}
};

class contextEvent : public event //own ContextEvents could be derived form this
{
public:
    contextEvent(eventHandler* h = nullptr, eventData* d = nullptr) : event(h, d) {}

    virtual unsigned int type() const override final { return eventType::context; }
    virtual unsigned int contextType() const = 0;
    virtual std::unique_ptr<event> clone() const = 0;
};


}
