#include <ny/backends/backend.hpp>

#include <ny/app/app.hpp>

#ifdef NY_WithWayland
#include <ny/backends/wayland/backend.hpp>
#endif // NY_WithGL

#ifdef NY_WithX11
#include <ny/backends/x11/backend.hpp>
#endif // WithX11

#ifdef NY_WithWinapi
#include <ny/backends/winapi/backend.hpp>
#endif // WithWinapi

namespace ny
{

std::vector<backend*> initBackends()
{
    std::vector<backend*> ret;

    #ifdef NY_WithWayland
    ret.push_back(new waylandBackend());
    #endif // WithWayland

    #ifdef NY_WithX11
    ret.push_back(new x11Backend());
    #endif // WithX11

    #ifdef NY_WithWinapi
    ret.push_back(new winapiBackend());
    #endif // WithWinpi

    return ret;
}


toplevelWindowContext* createToplevelWindowContext(toplevelWindow& win, const windowContextSettings& s)
{
    if(!getMainApp() || !getMainApp()->getBackend())
        return nullptr;

    return getMainApp()->getBackend()->createToplevelWindowContext(win, s);
}

childWindowContext* createChildWindowContext(childWindow& win, const windowContextSettings& s)
{
    if(!getMainApp() || !getMainApp()->getBackend())
        return nullptr;

    return getMainApp()->getBackend()->createChildWindowContext(win, s);
}

windowContext* createCustomWindowContext(window& win, const windowContextSettings& s)
{
    if(!getMainApp() || !getMainApp()->getBackend())
        return nullptr;

    return getMainApp()->getBackend()->createCustomWindowContext(win, s);
}


toplevelWC* createToplevelWC(toplevelWindow& win, const windowContextSettings& s){ return createToplevelWindowContext(win, s); };
childWC* createChildWC(childWindow& win, const windowContextSettings& s){ return createChildWindowContext(win, s); };
wc* createCustomWC(window& win, const windowContextSettings& s){ return createCustomWindowContext(win, s); };

}


