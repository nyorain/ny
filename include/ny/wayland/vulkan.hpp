#pragma once

#include <ny/wayland/include.hpp>
#include <ny/wayland/windowContext.hpp>

namespace ny
{

///WinapiWindowContext that also creates/owns a VkSurfaceKHR.
class WaylandVulkanWindowContext : public WaylandWindowContext
{
public:
	WaylandVulkanWindowContext(WaylandAppContext&, const WaylandWindowSettings&);
	~WaylandVulkanWindowContext();

	bool surface(Surface&) override;
	bool drawIntegration(WaylandDrawIntegration*) override { return false; }

	VkSurfaceKHR vkSurface() const { return vkSurface_; }
	VkInstance vkInstance() const { return vkInstance_; }

protected:
	VkSurfaceKHR vkSurface_ {};
	VkInstance vkInstance_ {};
};

}

#ifndef NY_WithVulkan
	#error ny was built without vulkan. Do not include this header.
#endif
