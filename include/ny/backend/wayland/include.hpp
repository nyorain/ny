#pragma once

#include <ny/include.hpp>

namespace ny
{
    class waylandAppContext;
    class waylandWindowContext;

    #ifdef NY_WithCairo
    class waylandCairoDrawContext;
    typedef waylandCairoDrawContext waylandCairoDC;
    #endif //Cairo

    #ifdef NY_WithGL
    class waylandEGLAppContext;
    class waylandEGLDrawContext;

    typedef waylandEGLAppContext waylandEGLAC;
    typedef waylandEGLDrawContext waylandEGLDC;
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
        class output;
    }
}

