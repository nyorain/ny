#include <ny/winapi/backend.hpp>
#include <ny/winapi/appContext.hpp>
#include <ny/winapi/windowContext.hpp>

namespace ny
{

winapiBackend::winapiBackend() : backend(Winapi)
{
}

appContext* winapiBackend::createAppContext()
{
    return new winapiAppContext();
}

toplevelWindowContext* winapiBackend::createToplevelWindowContext(toplevelWindow* win, const windowContextSettings& settings)
{
    winapiWindowContextSettings s;
    const winapiWindowContextSettings* sTest = dynamic_cast<const winapiWindowContextSettings*>(&settings);

    if(sTest)
    {
        s = *sTest;
    }
    else
    {
        s.hints = settings.hints;
    }

    return new winapiToplevelWindowContext(win, s);
}

childWindowContext* winapiBackend::createChildWindowContext(childWindow* win, const windowContextSettings& settings)
{
    if(settings.virtualState == virtuality::Should || settings.virtualState == virtuality::Must || settings.virtualState == virtuality::DontCare)
    {
        return new virtualWindowContext(win, settings);
    }

    winapiWindowContextSettings s;
    const winapiWindowContextSettings* sTest = dynamic_cast<const winapiWindowContextSettings*>(&settings);
    if(sTest)
    {
        s = *sTest;
    }
    else
    {
        s.hints = settings.hints;
    }

    return new winapiChildWindowContext(win, s);
}

}
