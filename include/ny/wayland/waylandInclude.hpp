#pragma once

#include <ny/include.hpp>

namespace ny
{
    class waylandAppContext;
    class waylandWindowContext;
    class waylandCairoContext;

    #ifdef NY_WithGL
    class waylandEGLAppContext;
    class waylandEGLContext;

    typedef waylandEGLAppContext waylandEGLAC;
    #endif // NY_WithGL

    typedef waylandAppContext waylandAC;
    typedef waylandWindowContext waylandWC;

    waylandAppContext* asWayland(appContext* c);
    waylandWindowContext* asWayland(windowContext* c);

    waylandAppContext* getWaylandAppContext();
    waylandAppContext* getWaylandAC();

    namespace wayland
    {
        class shmBuffer;
        class serverCallback;
    }
}

