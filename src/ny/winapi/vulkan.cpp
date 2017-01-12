// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/vulkan.hpp>
#include <ny/winapi/appContext.hpp>
#include <ny/surface.hpp>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

namespace ny {

WinapiVulkanWindowContext::WinapiVulkanWindowContext(WinapiAppContext& ac,
	const WinapiWindowSettings& ws) : WinapiWindowContext(ac, ws)
{
	vkInstance_ = ws.vulkan.instance;
	if(!vkInstance_)
		throw std::logic_error("ny::WinapiVulkanWindowContext: given VkInstance is invalid");

	VkWin32SurfaceCreateInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	info.hinstance = ac.hinstance();
	info.hwnd = handle();

	auto* allocCbs = ws.vulkan.allocationCallbacks;
	if(allocCbs) allocationCallbacks_ = std::make_unique<VkAllocationCallbacks>(*allocCbs);

	auto surface = reinterpret_cast<VkSurfaceKHR>(vkSurface_);
	auto res = vkCreateWin32SurfaceKHR(vkInstance_, &info, allocationCallbacks_.get(), &surface);
	if(res != VK_SUCCESS)
		throw std::runtime_error("ny::WinapiVulkanWindowContext: failed to create vulkan surface");

	if(ws.vulkan.storeSurface) *ws.vulkan.storeSurface = vkSurface_;
}

WinapiVulkanWindowContext::~WinapiVulkanWindowContext()
{
	auto surface = reinterpret_cast<VkSurfaceKHR>(vkSurface_);
	if(vkInstance_ && surface)
		vkDestroySurfaceKHR(vkInstance_, surface, allocationCallbacks_.get());
}

Surface WinapiVulkanWindowContext::surface()
{
	return {vkSurface_};
}

} // namespace ny
