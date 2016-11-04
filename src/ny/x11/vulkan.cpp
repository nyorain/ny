#include <ny/x11/vulkan.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/surface.hpp>

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

namespace ny
{

X11VulkanWindowContext::X11VulkanWindowContext(X11AppContext& ac,
	const X11WindowSettings& ws) : X11WindowContext(ac, ws)
{
	vkInstance_ = ws.vulkan.instance;
	if(!vkInstance_)
		throw std::logic_error("VulkanWinapiWC: setttings.vulkan.instance is a nullptr");

	VkXcbSurfaceCreateInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	info.connection = xConnection();
	info.window = xWindow();

	auto result = vkCreateXcbSurfaceKHR(vkInstance_, &info, nullptr, &vkSurface_);
	if(result != VK_SUCCESS)
		throw std::runtime_error("VulkanWinapiWC: failed to create vulkan surface");

	if(ws.vulkan.storeSurface) *ws.vulkan.storeSurface = vkSurface_;
}

X11VulkanWindowContext::~X11VulkanWindowContext()
{
	if(vkInstance_ && vkSurface_)
		vkDestroySurfaceKHR(vkInstance_, vkSurface_, nullptr);
}

bool X11VulkanWindowContext::surface(Surface& surface)
{
	surface.type = SurfaceType::vulkan;
	surface.vulkan = vkSurface_;
	return true;
}

}
