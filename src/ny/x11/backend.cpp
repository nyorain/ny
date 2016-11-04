#include <ny/backend/x11/backend.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/x11/appContext.hpp>

#ifdef NY_WithGL
 #include <ny/backend/x11/glx.hpp>
#endif //WithGL

#ifdef NY_WithVulkan
 #include <ny/backend/x11/vulkan.hpp>
#endif //WithVulkan

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

// std::unique_ptr<WindowContext> 
// X11Backend::createWindowContext(AppContext& ctx, const WindowSettings& s)
// {
// 	//window settings
//     X11WindowSettings settings;
//     const X11WindowSettings* ws = dynamic_cast<const X11WindowSettings*>(&s);
// 
//     if(ws) settings = *ws;
//     else settings.WindowSettings::operator=(s);
// 
// 	//appContext
// 	auto xac = dynamic_cast<X11AppContext*>(&ctx);
// 	if(!xac)
// 		throw std::invalid_argument("ny::X11Backend::createWC: invalid AppContext");
// 
// 	//type
// 	if(s.draw == DrawType::vulkan)
// 	{
// 		#ifdef NY_WithVulkan
// 		 return std::make_unique<X11VulkanWindowContext>(*xac, settings);
// 		#else
// 		 throw std::logic_error("ny::X11Backend::createWC: ny built without vulkan support");
// 		#endif
// 	}
// 	else if(s.draw == DrawType::gl)
// 	{
// 		#ifdef NY_WithGL	
// 		 return std::make_unique<GlxWindowContext>(*xac, settings);
// 		#else
// 		 throw std::logic_error("ny::X11Backend::createWC: ny built without GL suppport");
// 		#endif
// 	}
// 		
// 	return std::make_unique<X11WindowContext>(*xac, settings);
// }

}
