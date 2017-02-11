// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/vulkan.hpp>
#include <ny/android/appContext.hpp>
#include <ny/surface.hpp>

#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>

namespace ny {

AndroidVulkanWindowContext::AndroidVulkanWindowContext(AndroidAppContext& ac,
	const AndroidWindowSettings& ws) : AndroidWindowContext(ac, ws)
{
	vkInstance_ = ws.vulkan.instance;
	if(!vkInstance_)
		throw std::logic_error("ny::AndroidVulkanWindowContext: given VkInstance is invalid");

	VkAndroidSurfaceCreateInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	info.window = nativeWindow();

	auto* allocCbs = ws.vulkan.allocationCallbacks;
	if(allocCbs) allocationCallbacks_ = std::make_unique<VkAllocationCallbacks>(*allocCbs);

	auto& surface = reinterpret_cast<VkSurfaceKHR&>(vkSurface_);
	auto res = vkCreateAndroidSurfaceKHR(vkInstance_, &info, allocationCallbacks_.get(), &surface);
	if(res != VK_SUCCESS)
		throw std::runtime_error("ny::AndroidVulkanWindowContext: failed to create vulkan surface");

	if(ws.vulkan.storeSurface) *ws.vulkan.storeSurface = vkSurface_;
}

AndroidVulkanWindowContext::~AndroidVulkanWindowContext()
{
	auto surface = (VkSurfaceKHR)vkSurface_;
	if(vkInstance_ && surface)
		vkDestroySurfaceKHR(vkInstance_, surface, allocationCallbacks_.get());
}

Surface AndroidVulkanWindowContext::surface()
{
	return {vkSurface_};
}

} // namespace ny
