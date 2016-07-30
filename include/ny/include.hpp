#pragma once

#include <ny/fwd.hpp>
#include <ny/config.hpp>

namespace ny 
{ 

using namespace nytl; 
using namespace evg;
	
//backend typedefs
#ifdef NY_WithX11
 class X11Backend;
 class X11WindowContext;
 class X11AppContext;
#endif //WithX11

#ifdef NY_WithWayland
#endif //WithWayland

#ifdef NY_WithWinapi
 class WinapiBackend;
 class WinapiWindowContext;
 class WinapiAppContext;
#endif //WithWinapi

}
