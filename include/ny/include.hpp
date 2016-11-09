// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/config.hpp>

namespace ny
{

//to be removed in future.
//try to use the nytl namespace prefix everywhere.
using namespace nytl;

//backend typedefs
//XXX: move these and the config.hpp include to fwd.hpp?
//should they even be here?
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
