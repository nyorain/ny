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

#include <ny/window.hpp>
#include <ny/error.hpp>

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

std::unique_ptr<windowContext> backend::createWindowContext(window& win, const windowContextSettings& settings)
{
    childWindow* w = dynamic_cast<childWindow*>(&win);
    if(settings.virtualPref == preference::Must || (win.getParent() && win.getParent()->isVirtual()))
    {
        if(w)
        {
            return make_unique<virtualWindowContext>(*w, settings);
        }
        else
        {
            throw std::logic_error("backend::createWindowContext: virtual pref was set to Must for a toplevelWindow");
            return nullptr;
        }
    }
    else if(settings.virtualPref == preference::Should || settings.virtualPref == preference::DontCare)
    {
        if(w)
        {
            return make_unique<virtualWindowContext>(*w, settings);
        }
        else if(settings.virtualPref == preference::Should)
        {
            nyWarning("backend::createWindowContext: virtual pref was set to Should for a toplevelWindow");
        }
    }

    return createWindowContextImpl(win, settings);
}

}


