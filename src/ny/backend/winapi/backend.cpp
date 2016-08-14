#include <ny/backend/winapi/backend.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/winapi/gdi.hpp>
#include <ny/backend/winapi/wgl.hpp>

namespace ny
{

WinapiBackend WinapiBackend::instance_;

//
std::unique_ptr<AppContext> WinapiBackend::createAppContext()
{
    return std::make_unique<WinapiAppContext>();
}

// std::unique_ptr<WindowContext> WinapiBackend::createWindowContext(AppContext& context,
// 	const WindowSettings& settings)
// {
// 	auto wac = dynamic_cast<WinapiAppContext*>(&context);
// 	if(!wac)
// 	{
// 		throw std::logic_error("WinapiBackend::CreateWindow: invalid appContext type.");
// 	}
//
//     WinapiWindowSettings s;
//     const WinapiWindowSettings* sTest = dynamic_cast<const WinapiWindowSettings*>(&settings);
//
//     if(sTest)
//     {
//         s = *sTest;
//     }
//     else
//     {
//         auto& wsettings = static_cast<WindowSettings&>(s);
// 		wsettings = settings;
//     }
//
// 	if(s.draw == DrawType::none)
//     	return std::make_unique<WinapiWindowContext>(*wac, s);
// 	else if(s.draw == DrawType::dontCare || s.draw == DrawType::software)
// 		return std::make_unique<GdiWinapiWindowContext>(*wac, s);
// 	else
// 		return std::make_unique<WglWindowContext>(*wac, s);
// }

}
