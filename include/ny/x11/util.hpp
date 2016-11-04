#pragma once

#include <ny/x11/include.hpp>

#include <ny/keyboardContext.hpp>
#include <ny/mouseContext.hpp>
#include <ny/cursor.hpp>

#include <xcb/xcb.h>

// typedef struct xcb_visualtype_t xcb_visualtype_t;

namespace ny
{

///X11EventData stores the native xcb event for later use.
///To see where this might be neede look at the X11WC::beginResize and X11WC::beginMove functions.
class X11EventData : public EventData
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

namespace x11
{

///Special X11 backend event for reparenting.
///Event is needed since on reparenting the window position has to be set again.
using ReparentEvent = EventBase<eventType::x11::reparent>;
bool testCookie(xcb_connection_t& conn, const xcb_void_cookie_t& cookie, const char* msg = nullptr);

}

//utility conversions.
ImageDataFormat visualToFormat(const xcb_visualtype_t& visual, unsigned int depth);

//Both poorly implemented
//Int values instead of names
// int cursorToX11(CursorType cursor);
// CursorType x11ToCursor(int xcID);

}
