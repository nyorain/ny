#include <ny/x11/x11Backend.hpp>

#include <ny/x11/x11WindowContext.hpp>
#include <ny/x11/x11AppContext.hpp>
#include <ny/x11/x11Cairo.hpp>

#ifdef NY_WithGL
#include <ny/x11/glx.hpp>
#include <ny/x11/x11Egl.hpp>
#endif //WithGL

#include <X11/Xlib.h>

namespace ny
{

const x11Backend x11Backend::object;

x11Backend::x11Backend() : backend(X11)
{
}

bool x11Backend::isAvailable() const
{
    Display* dpy = XOpenDisplay(nullptr);

    if(!dpy)
    {
        return 0;
    }

    XCloseDisplay(dpy);

    return 1;
}

std::unique_ptr<appContext> x11Backend::createAppContext()
{
    return make_unique<x11AppContext>();
}

std::unique_ptr<windowContext> x11Backend::createWindowContextImpl(window& win, const windowContextSettings& s)
{
    x11WindowContextSettings settings;
    const x11WindowContextSettings* ws = dynamic_cast<const x11WindowContextSettings*> (&s);

    if(ws)
    {
        settings = *ws;
    }
    else
    {
        settings.hints &= s.hints;
        settings.virtualPref = s.virtualPref;
        settings.glPref = s.glPref;
    }

    return make_unique<x11WindowContext>(win, settings);
}

}
