#include <ny/wayland/waylandBackend.hpp>

#include <ny/wayland/waylandWindowContext.hpp>
#include <ny/wayland/waylandAppContext.hpp>
#include <ny/wayland/waylandCairo.hpp>

#ifdef NY_WithGL
#include <ny/wayland/waylandEgl.hpp>
#endif //WithGL

#include <wayland-client.h>

namespace ny
{

const waylandBackend waylandBackend::object;

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

    #ifdef NY_WithGL
    if(settings.glPref == preference::Must || settings.glPref == preference::Should)
    {
        return new waylandGLToplevelWindowContext(win, settings);
    }
    #endif // NY_WithGL

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

    #ifdef NY_WithGL
    if(settings.glPref == preference::Must || settings.glPref == preference::Should)
    {
        return new waylandGLChildWindowContext(win, settings);
    }
    #endif // NY_WithGL

    return new waylandCairoChildWindowContext(win, settings);
}

appContext* waylandBackend::createAppContext()
{
    return new waylandAppContext;
}


}
