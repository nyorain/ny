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
}

