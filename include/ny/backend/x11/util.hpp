#pragma once

#include <ny/backend/x11/include.hpp>

#include <ny/app/keyboard.hpp>
#include <ny/app/mouse.hpp>
#include <ny/window/cursor.hpp>

#include <xcb/xcb.h>

namespace ny
{

Mouse::Button x11ToButton(unsigned int id);
Keyboard::Key x11ToKey(unsigned int id);

int cursorToX11(Cursor::Type c);
Cursor::Type x11ToCursor(int xcID);

//x11Event
class X11EventData : public DeriveCloneable<EventData, X11EventData>
{
public:
    X11EventData(const xcb_generic_event_t& e) : event(e) {};
    xcb_generic_event_t event;
};


//reparent event
namespace eventType
{
	namespace x11
	{
		constexpr unsigned int reparent = 111;
	}
}

class X11ReparentEvent : public EventBase<eventType::x11::reparent, X11ReparentEvent>
{
public:
	using EvBase::EvBase;
};


}
