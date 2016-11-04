#pragma once

#include <ny/include.hpp>
#include <ny/event.hpp>
#include <ny/mouseButton.hpp>

#include <nytl/vec.hpp>
#include <nytl/callback.hpp>

namespace ny
{

///MouseContext interface, implemented by a backend.
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

public:
	///Will be called everytime a mouse button is clicked or released.
	nytl::Callback<void(MouseContext&, MouseButton, bool pressed)> onButton;

	///Will be called everytime the mouse moves.
	nytl::Callback<void(MouseContext&, const nytl::Vec2ui& pos, const nytl::Vec2ui& delta)> onMove;

	///Will be called everytime the pointer focus changes.
	///Note that both parameters might be a nullptr
	nytl::Callback<void(MouseContext&, WindowContext* prev, WindowContext* now)> onFocus;

	///Will be called everytime the mousewheel is rotated.
	///A value >0 means that the wheel was rotated forwards, a value < 0 backwards.
	nytl::Callback<void(MouseContext&, float value)> onWheel;
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
	nytl::Vec2i position;
};

///Event for a mouse move.
class MouseMoveEvent : public EventBase<eventType::mouseMove, MouseMoveEvent, 1>
{
public:
	using EvBase::EvBase;

	nytl::Vec2i position; //position in relation to the eventHandler
	nytl::Vec2i screenPosition;
	nytl::Vec2i delta;
};

///Event that will be send when the mouse crosses a window border, i.e. leaves or enters it.
class MouseCrossEvent : public EventBase<eventType::mouseCross, MouseCrossEvent>
{
public:
	using EvBase::EvBase;

	bool entered;
	nytl::Vec2i position;
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
