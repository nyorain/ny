#include <ny/backend/x11/backend.hpp>

#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/x11/appContext.hpp>

#include <X11/Xlib.h>

namespace ny
{

X11Backend X11Backend::instance_;

X11Backend::X11Backend()
{
}

bool X11Backend::available() const
{
    //XInitThreads(); //todo, make this optional
    Display* dpy = XOpenDisplay(nullptr);

    if(!dpy)
    {
        return 0;
    }

    XCloseDisplay(dpy);

    return 1;
}

std::unique_ptr<AppContext> X11Backend::createAppContext()
{
    return make_unique<X11AppContext>();
}

std::unique_ptr<WindowContext> 
X11Backend::createWindowContext(Window& win, const WindowContextSettings& s)
{
    X11WindowContextSettings settings;
    const X11WindowContextSettings* ws = dynamic_cast<const X11WindowContextSettings*>(&s);

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

    return make_unique<X11WindowContext>(win, settings);
}

}
