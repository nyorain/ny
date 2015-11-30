#pragma once

#include <ny/include.hpp>

namespace ny
{
    extern const unsigned int Winapi;

    class winapiWindowContext;
    class winapiAppContext;
    class gdiDrawContext;

    typedef winapiWindowContext winapiWC;
    typedef winapiAppContext winapiAC;
    typedef gdiDrawContext gdiDC;

    #ifdef NY_WithGL
    class wglDrawContext;

    typedef wglDrawContext wglDC;
    #endif // NY_WithGL

    winapiAppContext* nyWinapiAppContext();
    winapiAppContext* nyWinapiAC();
}
