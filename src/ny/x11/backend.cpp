#include <ny/x11/backend.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/x11/appContext.hpp>

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
    if(!dpy) return 0;
    XCloseDisplay(dpy);

    return 1;
}

std::unique_ptr<AppContext> X11Backend::createAppContext()
{
    return std::make_unique<X11AppContext>();
}


}
