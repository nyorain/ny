#pragma once

#include "waylandInclude.hpp"

namespace ny
{
    waylandAppContext* asWayland(appContext* c);
    waylandWindowContext* asWayland(windowContext* c);
    waylandToplevelWindowContext* asWayland(toplevelWindowContext* c);
    waylandChildWindowContext* asWayland(childWindowContext* c);
    waylandChildWindowContext* asWaylandChild(windowContext* c);
    waylandToplevelWindowContext* asWaylandToplevel(windowContext* c);

    waylandCairoToplevelWindowContext* asWaylandCairo(toplevelWindowContext* c);
    waylandCairoChildWindowContext* asWaylandCairo(childWindowContext* c);
    waylandCairoContext* asWaylandCairo(windowContext* c);

    #ifdef WithGL
    waylandGLToplevelWindowContext* asWaylandGL(toplevelWindowContext* c);
    waylandGLChildWindowContext* asWaylandGL(childWindowContext* c);
    waylandGLContext* asWaylandGL(windowContext* c);
    #endif // WithGL

}
