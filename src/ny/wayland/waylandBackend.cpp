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

windowContext* waylandBackend::createWindowContext(window& win, const windowContextSettings& s)
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

    return new waylandWindowContext(win, settings);
}

appContext* waylandBackend::createAppContext()
{
    return new waylandAppContext;
}


}
