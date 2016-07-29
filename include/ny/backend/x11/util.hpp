#pragma once

#include <ny/backend/x11/include.hpp>

#include <ny/app/keyboard.hpp>
#include <ny/app/mouse.hpp>
#include <ny/base/cursor.hpp>

#include <xcb/xcb.h>

namespace ny
{

//utility Ccnversions.
MouseButton x11ToButton(unsigned int id);
Key x11ToKey(unsigned int id);

int cursorToX11(Cursor::Type cursor);
Cursor::Type x11ToCursor(int xcID);

///X11EventData stores the native xcb event for later use.
///To see where this might be neede look at the X11WC::beginResize and X11WC::beginMove functions.
class X11EventData : public DeriveCloneable<EventData, X11EventData>
{
public:
    X11EventData(const xcb_generic_event_t& e) : event(e) {};
    xcb_generic_event_t event;
};

///Special X11 event ids.
namespace eventType
{
	namespace x11
	{
		constexpr unsigned int reparent = 1101;
	}
}

///Special X11 backend event for reparenting.
///Event is needed since on reparenting the window position has to be set again.
using X11ReparentEvent = EventBase<eventType::x11::reparent>;

}
