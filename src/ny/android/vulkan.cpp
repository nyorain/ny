// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/vulkan.hpp>
#include <ny/android/appContext.hpp>
#include <ny/surface.hpp>
#include <dlg/dlg.hpp>

#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>

namespace ny {

AndroidVulkanWindowContext::AndroidVulkanWindowContext(AndroidAppContext& ac,
	const AndroidWindowSettings& ws) : AndroidWindowContext(ac, ws)
{
	vkInstance_ = ws.vulkan.instance;
	if(!vkInstance_) {
		throw std::logic_error("ny::AndroidVulkanWindowContext: given VkInstance is invalid");
	}

	if(!nativeWindow()) {
		dlg_warn("no native window");
		if(ws.vulkan.storeSurface) *ws.vulkan.storeSurface = 0u;
		return;
	}

	VkAndroidSurfaceCreateInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	info.window = nativeWindow();

	auto* allocCbs = ws.vulkan.allocationCallbacks;
	if(allocCbs) allocCbs_ = std::make_unique<VkAllocationCallbacks>(*allocCbs);

	VkSurfaceKHR vkSurface;
	auto res = vkCreateAndroidSurfaceKHR(vkInstance_, &info, allocCbs_.get(), &vkSurface);
	if(res != VK_SUCCESS) {
		std::string msg = "ny::AndroidVulkanWindowContext: ";
		msg += "vkCreateAndroidSurfaceKHR error code ";
		msg += std::to_string(res);
		throw std::runtime_error(msg);
	}

	std::memcpy(&vkSurface_, &vkSurface, sizeof(vkSurface));
	if(ws.vulkan.storeSurface) {
		*ws.vulkan.storeSurface = vkSurface_;
	}
}

AndroidVulkanWindowContext::~AndroidVulkanWindowContext()
{
	if(vkSurface_) {
		vkDestroySurfaceKHR(vkInstance_, (VkSurfaceKHR) vkSurface_, allocCbs_.get());
	}
}

Surface AndroidVulkanWindowContext::surface()
{
	return {vkSurface_};
}

void AndroidVulkanWindowContext::nativeWindow(ANativeWindow* window)
{
	AndroidWindowContext::nativeWindow(window);

	if(vkSurface_) {
		SurfaceDestroyedEvent sde;
		listener().surfaceDestroyed(sde);

		vkDestroySurfaceKHR(vkInstance_, (VkSurfaceKHR) vkSurface_, allocCbs_.get());
		vkSurface_ = {};
	}

	if(window) {
		VkSurfaceKHR vkSurface;
		VkAndroidSurfaceCreateInfoKHR info {};
		info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
		info.window = nativeWindow();

		auto res = vkCreateAndroidSurfaceKHR(vkInstance_, &info, allocCbs_.get(), &vkSurface);
		if(res != VK_SUCCESS) {
			dlg_error("vkCreateAndroidSurfaceKHR error code {}", res);
		} else {
			std::memcpy(&vkSurface_, &vkSurface, sizeof(vkSurface));
			SurfaceCreatedEvent sce;
			sce.surface = {vkSurface_};
			listener().surfaceCreated(sce);
		}
	}
}

} // namespace ny
