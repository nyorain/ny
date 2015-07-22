#include <ny/include.hpp>

namespace ny
{

    class x11WindowContext;
    class x11AppContext;
    class x11CairoContext;

    typedef x11WindowContext x11WC;
    typedef x11AppContext x11AC;

    #ifdef NY_WithGL
    class glxContext;
    #endif //WithGL

    #ifdef NY_WithEGL
    class x11EGLContext;
    #endif //WithEGL

    namespace x11
    {
        class property;
    }

    x11AppContext* getX11AppContext();
    x11AppContext* getX11AC();
}
