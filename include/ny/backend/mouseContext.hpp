#pragma once

#include <ny/include.hpp>
#include <ny/event.hpp>

#include <nytl/vec.hpp>
#include <nytl/callback.hpp>

///Header can be used without linking to ny-backend.

namespace ny
{

///Contains all mouse buttons.
///Note that there might be no support for custom buttons one some backends.
enum class MouseButton : unsigned int
{
    none,
    left,
    right,
    middle,
	custom1,
    custom2,
    custom3,
    custom4,
	custom5,
	custom6
};

///Mouse interface.
class MouseContext
{
public:
	///Returns the newest mouse position that can be queried for the current window under
	///the pointer.
	///Note that if the pointer is not over a window, the result of this function is undefined.
	virtual Vec2ui position() const = 0;

	///Returns whether the given button is pressed.
	///This functions may be async to any received events and callbacks.
	virtual bool pressed(MouseButton button) const = 0;

	///Returns the WindowContext over that the pointer is located, or nullptr if there is none.
	virtual WindowContext* over() const = 0;

	///Will be called every time a mouse button is clicked or released.
	Callback<void(MouseButton button, bool pressed)> onButton;

	///Will be called every time the mouse moves.
	Callback<void(const Vec2ui& pos, const Vec2ui& delta)> onMove;

	///Will be called every time the pointer focus changes.
	///Note that both parameters might be a nullptr
	Callback<void(WindowContext* prev, WindowContext* now)> onFocus;
};

//Events
namespace eventType
{
	constexpr auto mouseMove = 21u;
	constexpr auto mouseButton = 22u;
	constexpr auto mouseWheel = 23u;
	constexpr auto mouseCross = 24u;
}

///Event for a mouse button press or release.
class MouseButtonEvent : public EventBase<eventType::mouseButton, MouseButtonEvent>
{
public:
	using EvBase::EvBase;

    bool pressed;
    MouseButton button;
    Vec2i position;
};

///Event for a mouse move.
class MouseMoveEvent : public EventBase<eventType::mouseMove, MouseMoveEvent, 1>
{
public:
	using EvBase::EvBase;

    Vec2i position; //position in relation to the eventHandler
    Vec2i screenPosition;
    Vec2i delta;
};

///Event that will be send when the mouse crosses a window border, i.e. leaves or enters it.
class MouseCrossEvent : public EventBase<eventType::mouseCross, MouseCrossEvent>
{
public:
	using EvBase::EvBase;

    bool entered;
    Vec2i position;
};

///Event that will be send when the mouse wheel is moved.
///Note that clicking the mouse wheel will simply generate a MouseButtonEvent.
class MouseWheelEvent : public EventBase<eventType::mouseWheel, MouseWheelEvent>
{
public:
	using EvBase::EvBase;
    float value;
};

}
