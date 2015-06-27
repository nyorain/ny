#pragma once

#include <ny/include.hpp>

namespace ny
{
    class waylandAppContext;
    class waylandWindowContext;
    class waylandToplevelWindowContext;
    class waylandChildWindowContext;

    class waylandCairoContext;
    class waylandCairoToplevelWindowContext;
    class waylandCairoChildWindowContext;

    typedef waylandAppContext waylandAC;
    typedef waylandWindowContext waylandWC;
    typedef waylandToplevelWindowContext waylandToplevelWC;
    typedef waylandChildWindowContext waylandChildWC;
    typedef waylandCairoToplevelWindowContext waylandCairoToplevelWC;
    typedef waylandCairoChildWindowContext waylandCairoChildWC;

    #ifdef NY_WithGL
    class waylandEGLAppContext;

    class waylandGLContext;
    class waylandGLToplevelWindowContext;
    class waylandGLChildWindowContext;

    typedef waylandGLToplevelWindowContext waylandGLToplevelWC;
    typedef waylandGLChildWindowContext waylandGLChildWC;
    #endif // NY_WithGL

    waylandAppContext* asWayland(appContext* c);
    waylandWindowContext* asWayland(windowContext* c);
    waylandToplevelWindowContext* asWayland(toplevelWindowContext* c);
    waylandChildWindowContext* asWayland(childWindowContext* c);
    waylandChildWindowContext* asWaylandChild(windowContext* c);
    waylandToplevelWindowContext* asWaylandToplevel(windowContext* c);

    waylandCairoToplevelWindowContext* asWaylandCairo(toplevelWindowContext* c);
    waylandCairoChildWindowContext* asWaylandCairo(childWindowContext* c);
    waylandCairoContext* asWaylandCairo(windowContext* c);

    #ifdef NY_WithGL
    waylandGLToplevelWindowContext* asWaylandGL(toplevelWindowContext* c);
    waylandGLChildWindowContext* asWaylandGL(childWindowContext* c);
    waylandGLContext* asWaylandGL(windowContext* c);
    #endif // NY_WithGL

    waylandAppContext* getWaylandAppContext();
}

