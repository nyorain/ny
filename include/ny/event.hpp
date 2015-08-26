#pragma once

#include <ny/include.hpp>

#include <ny/mouse.hpp>
#include <ny/keyboard.hpp>

#include <nyutil/vec.hpp>

namespace ny
{

namespace eventType
{
const unsigned char mouseMove = 1;
const unsigned char mouseButton = 2;
const unsigned char mouseWheel = 3;
const unsigned char mouseCross = 4;
const unsigned char key = 5;
const unsigned char destroy = 6;
}

enum class pressState
{
    released = 0,
    pressed = 1
};

enum class crossType
{
    left = 0,
    entered = 1
};


class eventData
{
public:
    virtual ~eventData(){}; //for dynamic cast
};

class event
{
public:
    event(unsigned int xtype);
    virtual ~event();

    template<class T> T& to()
    {
        return static_cast<T&>(*this);
    };

    const unsigned int type;
    unsigned int backend {0};
    eventHandler* handler {nullptr};
    eventData* data {nullptr}; //place for the backend to transport data (e.g. serial numbers for custom resize / move), todo: unique ptr
};


class destroyEvent : public event
{
public:
    destroyEvent() : event(eventType::destroy){};
};

class mouseButtonEvent : public event
{
public:
    mouseButtonEvent() : event(eventType::mouseButton) {};

    pressState state;
    mouse::button button;
    vec2i position;
};

class mouseMoveEvent : public event
{
public:
    mouseMoveEvent() : event(eventType::mouseMove) {};

    vec2i position;
    vec2i screenPosition;
    vec2i delta;
};

class mouseCrossEvent : public event
{
public:
    mouseCrossEvent() : event(eventType::mouseCross) {};

    crossType state;
    vec2i position;
};

class mouseWheelEvent : public event
{
public:
    mouseWheelEvent() : event(eventType::mouseWheel) {};

    float value;
};

class keyEvent : public event
{
public:
    keyEvent() : event(eventType::key) {};

    pressState state;
    keyboard::key key;
};

}

