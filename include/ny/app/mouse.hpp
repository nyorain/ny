#pragma once

#include <ny/include.hpp>
#include <ny/base/event.hpp>

#include <nytl/vec.hpp>
#include <nytl/callback.hpp>
#include <nytl/compFunc.hpp>

#include <bitset>

namespace ny
{

//mouse
class Mouse
{
public:
    enum class Button : int
    {
        none = -1,
        left = 0,
        right,
        middle,
        custom1,
        custom2,
        custom3,
        custom4,
    };
};

//events
namespace eventType
{
	constexpr unsigned int mouseMove = 3;
	constexpr unsigned int mouseButton = 4;
	constexpr unsigned int mouseWheel = 5;
	constexpr unsigned int mouseCross = 6;
}

class MouseButtonEvent : public EventBase<eventType::mouseButton, MouseButtonEvent>
{
public:
	using EvBase::EvBase;

    bool pressed;
    Mouse::Button button;
    Vec2i position;
};

//really overrideable? delta!
class MouseMoveEvent : public EventBase<eventType::mouseMove, MouseMoveEvent, 1>
{
public:
	using EvBase::EvBase;

    Vec2i position; //position in relation to the eventHandler
    Vec2i screenPosition;
    Vec2i delta;
};

class MouseCrossEvent : public EventBase<eventType::mouseCross, MouseCrossEvent>
{
public:
	using EvBase::EvBase;

    bool entered;
    Vec2i position;
};

class MouseWheelEvent : public EventBase<eventType::mouseWheel, MouseWheelEvent>
{
public:
	using EvBase::EvBase;
    float value;
};

}
