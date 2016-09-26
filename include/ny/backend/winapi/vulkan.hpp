#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <vulkan/vulkan.h>

namespace ny
{

///WinapiWindowContext that also creates/owns a VkSurfaceKHR.
class VulkanWinapiWindowContext : public WinapiWindowContext
{
public:
	VulkanWinapiWindowContext(WinapiAppContext& ac, WindowSettings& ws);
	~VulkanWinapiWindowContext();

	VkSurfaceKHR vkSurface() const { return vkSurface_; }

protected:
	VkSurfaceKHR vkSurface_;
	VulkanContext* context_;
};

}

#ifndef NY_WithVulkan
	#error ny was built without vulkan. Do not include this header.
#endif
