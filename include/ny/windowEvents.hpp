#pragma once

#include <ny/include.hpp>
#include <ny/event.hpp>

namespace ny
{

namespace eventType
{
const unsigned char windowSize = 11;
const unsigned char windowPosition = 12;
const unsigned char windowDraw = 13;
const unsigned char windowClose = 14;
const unsigned char windowShow = 15;
const unsigned char windowFocus = 16;
const unsigned char context = 20;
}

enum class focusState
{
    lost = 0,
    gained = 1
};

class focusEvent : public event
{
public:
    focusEvent() : event(eventType::windowFocus) { };
    focusState state;
};

class sizeEvent : public event
{
public:
    sizeEvent() : event(eventType::windowSize) {};
    vec2ui size;
};

class positionEvent : public event
{
public:
    positionEvent() : event(eventType::windowPosition) {};
    vec2ui position;
};

class drawEvent : public event
{
public:
    drawEvent() : event(eventType::windowDraw) {};
};

class contextEvent : public event //own ContextEvents could be derived form this
{
public:
    contextEvent(unsigned int pbackend, unsigned int ptype) : event(eventType::context), contextEventType(ptype) { backend = pbackend; };

    unsigned int contextEventType; //type of the event from the context/backend
};


}
