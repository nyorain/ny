#include <ny/include.hpp>

namespace ny
{

    class x11WindowContext;
    class x11AppContext;

    typedef x11WindowContext x11WC;
    typedef x11AppContext x11AC;

    #ifdef NY_WithCairo
    class x11CairoDrawContext;
    typedef x11CairoDrawContext x11CairoDC;
    #endif // Cairo

    #ifdef NY_WithGL
    class glxDrawContext;
    typedef glxDrawContext glxDC;
    #endif //WithGL

    #ifdef NY_WithEGL
    class x11EGLDrawContext;
    typedef x11EGLDrawContext x11EGLDC;
    #endif //WithEGL

    namespace x11
    {
        class property;
    }

    x11AppContext* getX11AppContext();
    x11AppContext* getX11AC();
}
