#include <ny/winapi/winapiBackend.hpp>
#include <ny/winapi/winapiAppContext.hpp>
#include <ny/winapi/winapiWindowContext.hpp>

namespace ny
{

winapiBackend::winapiBackend() : backend(Winapi)
{
}

appContext* winapiBackend::createAppContext()
{
    return new winapiAppContext();
}

windowContext* winapiBackend::createWindowContext(window& win, const windowContextSettings& settings)
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

    return new winapiWindowContext(win, s);
}

}
