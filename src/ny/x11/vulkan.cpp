// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/vulkan.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/surface.hpp>

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

namespace ny {

X11VulkanWindowContext::X11VulkanWindowContext(X11AppContext& ac,
	const X11WindowSettings& ws) : X11WindowContext(ac, ws)
{
	vkInstance_ = ws.vulkan.instance;
	if(!vkInstance_)
		throw std::logic_error("ny::X11VulkanWindowContext: given VkInstance is invalid");

	VkXcbSurfaceCreateInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	info.connection = &xConnection();
	info.window = xWindow();

	auto* allocCbs = ws.vulkan.allocationCallbacks;
	if(allocCbs) allocationCallbacks_ = std::make_unique<VkAllocationCallbacks>(*allocCbs);

	auto& surface = reinterpret_cast<VkSurfaceKHR&>(vkSurface_);
	auto res = vkCreateXcbSurfaceKHR(vkInstance_, &info, allocationCallbacks_.get(), &surface);
	if(res != VK_SUCCESS)
		throw std::runtime_error("ny::X11VulkanWindowContext: failed to create vulkan surface");

	if(ws.vulkan.storeSurface) *ws.vulkan.storeSurface = vkSurface_;
}

X11VulkanWindowContext::~X11VulkanWindowContext()
{
	auto surface = reinterpret_cast<VkSurfaceKHR>(vkSurface_);
	if(vkInstance_ && surface)
		vkDestroySurfaceKHR(vkInstance_, surface, allocationCallbacks_.get());
}

Surface X11VulkanWindowContext::surface()
{
	return {vkSurface_};
}

} // namespace ny
