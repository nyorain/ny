#include <ny/backend.hpp>

#include <ny/app.hpp>

#ifdef NY_WithWayland
#include <ny/wayland/waylandBackend.hpp>
#endif // NY_WithGL

#ifdef NY_WithX11
#include <ny/x11/x11Backend.hpp>
#endif // WithX11

#ifdef NY_WithWinapi
#include <ny/winapi/winapiBackend.hpp>
#endif // WithWinapi

namespace ny
{

windowContext* createWindowContext(window& win, const windowContextSettings& s)
{
    if(!getMainApp() || !getMainApp()->getBackend())
        return nullptr;

    return getMainApp()->getBackend()->createWindowContext(win, s);
}

wc* createCustomWC(window& win, const windowContextSettings& s){ return createWindowContext(win, s); };


backend::backend(unsigned int i) : id(i)
{
    app::registerBackend(*this);
}

}


