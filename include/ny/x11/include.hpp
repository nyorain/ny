#pragma once

#include <ny/include.hpp>

#ifndef NY_WithX11
	#error ny was built without X11. Do not include this header file!
#endif //WithX11

//xlib/xcb typedefs
typedef struct _XDisplay Display;
typedef struct xcb_connection_t xcb_connection_t;

namespace ny
{
    class X11WindowContext;
    class X11AppContext;
	class X11MouseContext;
	class X11KeyboardContext;
	class X11DataManager;
	class X11ErrorCategory;
    // class GlxContext;

	namespace x11
	{
		///Dummy delcaration to not include xcb_ewmh.h
		///The xcb_ewmh_connection_t type cannot be forward declared since it is a unnamed
		///struct typedef in the original xcb_ewmh header, which should not be included in a
		///header file.
		struct EwmhConnection;
	}
}
