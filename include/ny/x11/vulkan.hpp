#pragma once

#include <ny/x11/include.hpp>
#include <ny/x11/windowContext.hpp>

namespace ny
{

///WinapiWindowContext that also creates/owns a VkSurfaceKHR.
class X11VulkanWindowContext : public X11WindowContext
{
public:
	X11VulkanWindowContext(X11AppContext&, const X11WindowSettings&);
	~X11VulkanWindowContext();

	bool surface(Surface&) override;
	bool drawIntegration(X11DrawIntegration*) override { return false; }

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
