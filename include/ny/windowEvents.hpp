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
constexpr unsigned int windowClose = 14;
constexpr unsigned int windowShow = 15;
constexpr unsigned int windowFocus = 16;
constexpr unsigned int windowRefresh = 17;
constexpr unsigned int context = 20;
}

class focusEvent : public eventBase<eventType::windowFocus>
{
public:
    focusEvent(eventHandler* h = nullptr, bool gain = 0, eventData* d = nullptr)
        : evBase(h, d), focusGained(gain) {}

    bool focusGained;
};

class sizeEvent : public eventBase<eventType::windowSize>
{
public:
    sizeEvent(eventHandler* h = nullptr, vec2ui s = vec2ui(), bool ch = 1, eventData* d = nullptr)
        : evBase(h, d), size(s), change(ch) {}

    vec2ui size;
    bool change;
};

class positionEvent : public eventBase<eventType::windowPosition>
{
public:
    positionEvent(eventHandler* h = nullptr, vec2i pos = vec2i(), eventData* d = nullptr)
        : evBase(h, d), position(pos) {}

    vec2i position {0, 0};
};

class drawEvent : public eventBase<eventType::windowDraw>
{
public:
    drawEvent(eventHandler* h = nullptr, eventData* d = nullptr) : evBase(h, d) {}
};

class refreshEvent : public eventBase<eventType::windowRefresh>
{
public:
    refreshEvent(eventHandler* h = nullptr, eventData* d = nullptr) : evBase(h, d) {}
};

class contextEvent : public eventBase<eventType::context> //own ContextEvents could be derived form this
{
public:
    contextEvent(eventHandler* h = nullptr, eventData* d = nullptr) : evBase(h, d) {}
    virtual unsigned int getContextEventType() const = 0;
};


}
