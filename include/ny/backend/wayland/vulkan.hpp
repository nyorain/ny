#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <vulkan/vulkan.h>

namespace ny
{

// class WaylandVulkanWindowContext : public WaylandWindowContext
// {
// public:
// 	WaylandVulkanWindowContext(WaylandAppContext& ac, const WaylandWindowSettings& settings = {});
// 	~WaylandVulkanWindowContext();
// 
// protected:
// 	VkSurfaceKHR vkSurface_;
// 
// 	std::unique_ptr<evg::VulkanDrawContext> drawContext_;
// };

}
