#include "backends/wayland/backend.hpp"

#include "backends/wayland/windowContext.hpp"
#include "backends/wayland/appContext.hpp"
#include "backends/wayland/cairo.hpp"

#ifdef WithGL
#include "backends/wayland/gl.hpp"
#endif //WithGL

#include <wayland-client.h>

namespace ny
{

waylandBackend::waylandBackend() : backend(Wayland)
{

}

bool waylandBackend::isAvailable() const
{
    wl_display* disp = wl_display_connect(nullptr);

    if(!disp)
    {
        return 0;
    }

    wl_display_disconnect(disp);

    return 1;
}

toplevelWindowContext* waylandBackend::createToplevelWindowContext(toplevelWindow& win, const windowContextSettings& s)
{
    waylandWindowContextSettings settings;
    const waylandWindowContextSettings* ws = dynamic_cast<const waylandWindowContextSettings*> (&s);

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

    #ifdef WithGL
    if(settings.glPref == preference::DontCare || settings.glPref == preference::Must || settings.glPref == preference::Should)
    {
        return new waylandGLToplevelWindowContext(win, settings);
    }
    #endif // WithGL

    return new waylandCairoToplevelWindowContext(win, settings);
}

childWindowContext* waylandBackend::createChildWindowContext(childWindow& win, const windowContextSettings& s)
{
    if(s.virtualPref == preference::Should || s.virtualPref == preference::Must || s.virtualPref == preference::DontCare)
    {
        return new virtualWindowContext(win, s);
    }

    waylandWindowContextSettings settings;
    const waylandWindowContextSettings* ws = dynamic_cast<const waylandWindowContextSettings*> (&s);

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

    #ifdef WithGL
    if(settings.glPref == preference::DontCare || settings.glPref == preference::Must || settings.glPref == preference::Should)
    {
        return new waylandGLChildWindowContext(win, settings);
    }
    #endif // WithGL

    return new waylandCairoChildWindowContext(win, settings);
}

appContext* waylandBackend::createAppContext()
{
    return new waylandAppContext;
}


}
