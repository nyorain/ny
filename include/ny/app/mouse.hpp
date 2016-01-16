#pragma once

#include <ny/app/include.hpp>
#include <ny/app/event.hpp>

#include <nytl/vec.hpp>
#include <nytl/callback.hpp>
#include <nytl/compFunc.hpp>

#include <bitset>

namespace ny
{

//grab
//TODO
struct MouseGrab
{
    EventHandler* handler_ {nullptr};
    std::unique_ptr<Event> event_ {nullptr};
};

//mouse
class Mouse
{

friend class App;

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

protected:
    static std::bitset<8> states_;
    static vec2i position_;

    //static mouseGrab grab_ {};

protected:
    static void buttonPressed(Button button, bool pressed);
    static void wheelMoved(float value);
    static void position(const vec2i& pos);

    static callback<void(const vec2i&)> moveCallback_;
    static callback<void(Button, bool)> buttonCallback_;
    static callback<void(float)> wheelCallback_;

public:
    static bool buttonPressed(Button button);
    static vec2i position();

    template<typename F> connection onMove(F&& func) { return moveCallback_.add(func); }
    template<typename F> connection onButton(F&& func) { return buttonCallback_.add(func); }
    template<typename F> connection onWheel(F&& func) { return wheelCallback_.add(func); }
};

//events
namespace eventType
{
constexpr unsigned int mouseMove = 3;
constexpr unsigned int mouseButton = 4;
constexpr unsigned int mouseWheel = 5;
constexpr unsigned int mouseCross = 6;
}

class MouseButtonEvent : public EventBase<MouseButtonEvent, eventType::mouseButton>
{
public:
	using EvBase::EvBase;

    bool pressed;
    Mouse::Button button;
    vec2i position;
};

class MouseMoveEvent : public EventBase<MouseMoveEvent, eventType::mouseMove, 1>
{
public:
	using EvBase::EvBase;

    vec2i position;
    vec2i screenPosition;
    vec2i delta;
};

class MouseCrossEvent : public EventBase<MouseCrossEvent, eventType::mouseCross>
{
public:
	using EvBase::EvBase;

    bool entered;
    vec2i position;
};

class MouseWheelEvent : public EventBase<MouseWheelEvent, eventType::mouseWheel>
{
public:
	using EvBase::EvBase;
    float value;
};


}
