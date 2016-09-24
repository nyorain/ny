#pragma once

#include <ny/fwd.hpp>
#include <ny/config.hpp>

namespace ny
{

//to be removed in future.
//try to use the nytl namespace prefix everywhere.
using namespace nytl;

//backend typedefs
#ifdef NY_WithX11
 class X11Backend;
 class X11WindowContext;
 class X11AppContext;
#endif //WithX11

#ifdef NY_WithWayland
 class WaylandBackend;
 class WaylandWindowContext;
 class WaylandAppContext;
#endif //WithWayland

#ifdef NY_WithWinapi
 class WinapiBackend;
 class WinapiWindowContext;
 class WinapiAppContext;
#endif //WithWinapi

}
