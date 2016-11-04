#pragma once

#include <ny/include.hpp>
#include <ny/base/event.hpp>
#include <nytl/vec.hpp>

namespace ny
{

namespace eventType
{
	constexpr auto size = 11u;
	constexpr auto position = 12u;
	constexpr auto draw = 13u;
	constexpr auto show = 14u;
	constexpr auto close = 17u;
}

///Signals that a window was resized.
class SizeEvent : public EventBase<eventType::size, SizeEvent>
{
public:
	using EvBase::EvBase;
    Vec2ui size {0, 0};
};

///Signals that a widow was shown/hidden or its show state was changed by the window manager.
class ShowEvent : public EventBase<eventType::show, ShowEvent>
{
public:
	using EvBase::EvBase;

    bool show = 0;
	ToplevelState state;
};

///Signals that the window was moved from the window manager side.
class PositionEvent : public EventBase<eventType::position, PositionEvent, 1>
{
public:
	using EvBase::EvBase;
    Vec2i position {0, 0};
};

///Signals that a window should be redrawn.
using DrawEvent = EventBase<eventType::draw, void, 1>;

///Signals that a window received a close event from the window manager.
using CloseEvent = EventBase<eventType::close, void, 1>;

}
