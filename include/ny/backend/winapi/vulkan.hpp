#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>

namespace ny
{

///WinapiWindowContext that also creates/owns a VkSurfaceKHR.
class VulkanWinapiWindowContext : public WinapiWindowContext
{
public:
	VulkanWinapiWindowContext(WinapiAppContext&, const WinapiWindowSettings&);
	~VulkanWinapiWindowContext();

	bool surface(Surface&) override;
	bool drawIntegration(WinapiDrawIntegration*) override { return false; }

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
