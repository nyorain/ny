#pragma once

#include <ny/include.hpp>
#include <ny/base/event.hpp>

#include <nytl/vec.hpp>
#include <nytl/clone.hpp>

#include <memory>

namespace ny
{

namespace eventType
{
	constexpr auto windowSize = 11u;
	constexpr auto windowPosition = 12u;
	constexpr auto windowDraw = 13u;
	constexpr auto windowShow = 14u;
	constexpr auto windowFocus = 15u;
	constexpr auto windowRefresh = 16u;
	constexpr auto windowClose = 17u;
}

//
class SizeEvent : public EventBase<eventType::windowSize, SizeEvent>
{
public:
	using EvBase::EvBase;
    Vec2ui size {0, 0};
};

class ShowEvent : public EventBase<eventType::windowShow, ShowEvent>
{
public:
	using EvBase::EvBase;

    bool show = 0;
	ToplevelState state;
};

class CloseEvent : public EventBase<eventType::windowClose, CloseEvent>
{
public:
	using EvBase::EvBase;
};

class PositionEvent : public EventBase<eventType::windowPosition, PositionEvent, 1>
{
public:
	using EvBase::EvBase;
    Vec2i position {0, 0};
};

class DrawEvent : public EventBase<eventType::windowDraw, DrawEvent, 1>
{
public:
	using EvBase::EvBase;
};

class RefreshEvent : public EventBase<eventType::windowRefresh, RefreshEvent, 1>
{
public:
	using EvBase::EvBase;
};

}
