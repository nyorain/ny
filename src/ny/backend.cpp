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

std::unique_ptr<windowContext> createWindowContext(window& win, const windowContextSettings& s)
{
    if(!nyMainApp() || !nyMainApp()->getBackend())
        return nullptr;

    return nyMainApp()->getBackend()->createWindowContext(win, s);
}

std::unique_ptr<wc> createWC(window& win, const windowContextSettings& s){ return createWindowContext(win, s); };


backend::backend(unsigned int i) : id(i)
{
    app::registerBackend(*this);
}

}


