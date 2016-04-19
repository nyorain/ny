#include <ny/backend/winapi/backend.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/winapi/windowContext.hpp>

namespace ny
{

WinapiBackend WinapiBackend::instance_;

//
std::unique_ptr<AppContext> WinapiBackend::createAppContext()
{
    return std::make_unique<WinapiAppContext>();
}

std::unique_ptr<WindowContext> WinapiBackend::createWindowContext(AppContext& context,
	const WindowSettings& settings)
{
	auto wac = dynamic_cast<WinapiAppContext*>(&context);
	if(!wac)
	{
		throw std::logic_error("WinapiBackend::CreateWindow: invalid appContext type.");
	}

    WinapiWindowSettings s;
    const WinapiWindowSettings* sTest = dynamic_cast<const WinapiWindowSettings*>(&settings);

    if(sTest)
    {
        s = *sTest;
    }
    else
    {
        auto& wsettings = static_cast<WindowSettings&>(s);
		wsettings = settings;
    }

    return std::make_unique<WinapiWindowContext>(*wac, s);
}

}
