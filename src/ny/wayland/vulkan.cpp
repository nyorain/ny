// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/vulkan.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/surface.hpp>

#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

namespace ny
{

WaylandVulkanWindowContext::WaylandVulkanWindowContext(WaylandAppContext& ac,
	const WaylandWindowSettings& ws) : WaylandWindowContext(ac, ws)
{
	vkInstance_ = ws.vulkan.instance;
	if(!vkInstance_)
		throw std::logic_error("ny::WaylandVulkanWindowContext: given VkInstance is invalid");

	VkWaylandSurfaceCreateInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	info.display = &wlDisplay();
	info.surface = &wlSurface();

	auto* allocCbs = ws.vulkan.allocationCallbacks;
	if(allocCbs) allocationCallbacks_ = std::make_unique<VkAllocationCallbacks>(*allocCbs);

	auto surface = reinterpret_cast<VkSurfaceKHR>(vkSurface_);
	auto res = vkCreateWaylandSurfaceKHR(vkInstance_, &info, allocationCallbacks_.get(), &surface);
	if(res != VK_SUCCESS)
		throw std::runtime_error("ny::WaylandVulkanWindowContext: failed to create vulkan surface");

	if(ws.vulkan.storeSurface) *ws.vulkan.storeSurface = vkSurface_;
}

WaylandVulkanWindowContext::~WaylandVulkanWindowContext()
{
	auto surface = reinterpret_cast<VkSurfaceKHR>(vkSurface_);
	if(vkInstance_ && surface)
		vkDestroySurfaceKHR(vkInstance_, surface, allocationCallbacks_.get());
}

Surface WaylandVulkanWindowContext::surface()
{
	return {vkSurface_};
}

}
