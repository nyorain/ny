#include "backends/x11/backend.hpp"

#include "backends/x11/windowContext.hpp"
#include "backends/x11/appContext.hpp"
#include "backends/x11/cairo.hpp"

#ifdef WithGL
#include "backends/x11/glx.hpp"
#include "backends/x11/egl.hpp"
#endif //WithGL

#include <X11/Xlib.h>

namespace ny
{


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

appContext* x11Backend::createAppContext()
{
    return new x11AppContext();
}

toplevelWindowContext* x11Backend::createToplevelWindowContext(toplevelWindow& win, const windowContextSettings& s)
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

    #ifdef WithGL
    if(settings.glPref == preference::DontCare || settings.glPref == preference::Must || settings.glPref == preference::Should)
    {
        glxWindowContextSettings glxSettings;
        const glxWindowContextSettings* glxws = dynamic_cast<const glxWindowContextSettings*> (&s);

        if(glxws)
        {
            glxSettings = *glxws;
        }
        else
        {
            glxSettings.hints &= settings.hints;
            glxSettings.virtualPref = settings.virtualPref;
            glxSettings.glPref = settings.glPref;
        }

        return new glxToplevelWindowContext(win, glxSettings);

        //not av. now
        //return new x11EGLToplevelWindowContext(win, settings);
    }
    #endif // WithGL

    return new x11CairoToplevelWindowContext(win, settings);
}

childWindowContext* x11Backend::createChildWindowContext(childWindow& win, const windowContextSettings& s)
{
    if(s.virtualPref == preference::Should || s.virtualPref == preference::Must || s.virtualPref == preference::DontCare)
    {
        return new virtualWindowContext(win, s);
    }

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

    #ifdef WithGL
    if(settings.glPref == preference::DontCare || settings.glPref == preference::Must || settings.glPref == preference::Should)
    {
        glxWindowContextSettings glxSettings;
        const glxWindowContextSettings* glxws = dynamic_cast<const glxWindowContextSettings*> (&s);

        if(glxws)
        {
            glxSettings = *glxws;
        }
        else
        {
            glxSettings.hints &= settings.hints;
            glxSettings.virtualPref = settings.virtualPref;
            glxSettings.glPref = settings.glPref;
        }

        return new glxChildWindowContext(win, glxSettings);

        //not av. now
        //return new x11EGLToplevelWindowContext(win, settings);
    }
    #endif // WithGL

    return new x11CairoChildWindowContext(win, settings);
}


}
