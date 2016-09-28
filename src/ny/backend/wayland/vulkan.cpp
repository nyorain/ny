#define VK_USE_PLATFORM_WAYLAND_KHR

#include <ny/backend/wayland/vulkan.hpp>
#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/integration/surface.hpp>
#include <vulkan/vulkan.h>

namespace ny
{

WaylandVulkanWindowContext::WaylandVulkanWindowContext(WaylandAppContext& ac,
	const WaylandWindowSettings& ws) : WaylandWindowContext(ac, ws)
{
	vkInstance_ = ws.vulkan.instance;
	if(!vkInstance_)
		throw std::logic_error("VulkanWinapiWC: setttings.vulkan.instance is a nullptr");

	VkWaylandSurfaceCreateInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	info.display = &wlDisplay();
	info.surface = &wlSurface();

	auto result = vkCreateWaylandSurfaceKHR(vkInstance_, &info, nullptr, &vkSurface_);
	if(result != VK_SUCCESS)
		throw std::runtime_error("VulkanWinapiWC: failed to create vulkan surface");

	if(ws.vulkan.storeSurface) *ws.vulkan.storeSurface = vkSurface_;
}

WaylandVulkanWindowContext::~WaylandVulkanWindowContext()
{
	if(vkInstance_ && vkSurface_)
		vkDestroySurfaceKHR(vkInstance_, vkSurface_, nullptr);
}

bool WaylandVulkanWindowContext::surface(Surface& surface)
{
	surface.type = SurfaceType::vulkan;
	surface.vulkan = vkSurface_;
	return true;
}

}
