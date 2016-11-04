#define VK_USE_PLATFORM_WIN32_KHR

#include <ny/backend/winapi/vulkan.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/integration/surface.hpp>
#include <vulkan/vulkan.h>

namespace ny
{

WinapiVulkanWindowContext::WinapiVulkanWindowContext(WinapiAppContext& ac,
	const WinapiWindowSettings& ws) : WinapiWindowContext(ac, ws)
{
	vkInstance_ = ws.vulkan.instance;
	if(!vkInstance_)
		throw std::logic_error("VulkanWinapiWC: setttings.vulkan.instance is a nullptr");

	VkWin32SurfaceCreateInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	info.hinstance = ac.hinstance();
	info.hwnd = handle();

	auto result = vkCreateWin32SurfaceKHR(vkInstance_, &info, nullptr, &vkSurface_);
	if(result != VK_SUCCESS)
		throw std::runtime_error("VulkanWinapiWC: failed to create vulkan surface");

	if(ws.vulkan.storeSurface) *ws.vulkan.storeSurface = vkSurface_;
}

WinapiVulkanWindowContext::~WinapiVulkanWindowContext()
{
	if(vkInstance_ && vkSurface_)
		vkDestroySurfaceKHR(vkInstance_, vkSurface_, nullptr);
}

bool WinapiVulkanWindowContext::surface(Surface& surface)
{
	surface.type = SurfaceType::vulkan;
	surface.vulkan = vkSurface_;
	return true;
}

}
