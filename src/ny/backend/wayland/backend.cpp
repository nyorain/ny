#include <ny/backend/wayland/backend.hpp>

#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/appContext.hpp>


#include <wayland-client-core.h>

namespace ny
{

WaylandBackend WaylandBackend::instance_;

WaylandBackend::WaylandBackend()
{
}

bool WaylandBackend::available() const
{
    wl_display* dpy = wl_display_connect(nullptr);
    if(!dpy) return false;
    wl_display_disconnect(dpy);
    return true;
}

AppContextPtr WaylandBackend::createAppContext()
{
    return std::make_unique<WaylandAppContext>();
}

// WindowContextPtr WaylandBackend::createWindowContext(AppContext& ctx, const WindowSettings& sttngs)
// {
//     WaylandWindowSettings settings;
//     const auto* ws = dynamic_cast<const WaylandWindowSettings*>(&sttngs);
// 	auto* wlac = dynamic_cast<WaylandAppContext*>(&ctx);
// 
// 	if(!wlac)
// 		throw std::invalid_argument("ny::WaylandBackend::createWC: invalid AppContext");
// 
//     if(ws) settings = *ws;
//     else settings.WindowSettings::operator=(sttngs);
// 
// 	//type
// 	if(s.draw == DrawType::vulkan)
// 	{
// 		#ifdef NY_WithVulkan
// 		 return std::make_unique<WaylandVulkanWindowContext>(*xac, settings);
// 		#else
// 		 throw std::logic_error("ny::X11Backend::createWC: ny built without vulkan support");
// 		#endif
// 	}
// 	else if(s.draw == DrawType::gl)
// 	{
// 		#ifdef NY_WithGL	
// 		 return std::make_unique<WaylandEglWindowContext>(*xac, settings);
// 		#else
// 		 throw std::logic_error("ny::X11Backend::createWC: ny built without GL suppport");
// 		#endif
// 	}
// 		
// 	return std::make_unique<WaylandWindowContext>(*xac, settings);
// }


}
