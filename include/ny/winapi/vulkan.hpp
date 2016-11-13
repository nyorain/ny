#pragma once

#include <ny/winapi/include.hpp>
#include <ny/winapi/windowContext.hpp>

namespace ny
{

///WinapiWindowContext that also creates/owns a VkSurfaceKHR.
class WinapiVulkanWindowContext : public WinapiWindowContext
{
public:
	WinapiVulkanWindowContext(WinapiAppContext&, const WinapiWindowSettings&);
	~WinapiVulkanWindowContext();

	Surface surface() override;

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
