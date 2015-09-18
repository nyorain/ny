#include <ny/winapi/winapiBackend.hpp>
#include <ny/winapi/winapiAppContext.hpp>
#include <ny/winapi/winapiWindowContext.hpp>

namespace ny
{

winapiBackend winapiBackend::object{};

//
winapiBackend::winapiBackend() : backend(Winapi)
{
}

std::unique_ptr<appContext> winapiBackend::createAppContext()
{
    return std::make_unique<winapiAppContext>();
}

std::unique_ptr<windowContext> winapiBackend::createWindowContextImpl(window& win, const windowContextSettings& settings)
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

    return std::make_unique<winapiWindowContext>(win, s);
}

}
