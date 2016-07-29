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
    class X11CairoDrawContext;
    class GlxContext;

	namespace x11
	{
	}
}
