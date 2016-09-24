#pragma once

#include <ny/include.hpp>
#include <vulkan/vulkan.h>
#include <vector>

namespace ny
{

///Information about a vulkan queue associated with a device.
struct VulkanQueueInfo
{
	VkQueue queue;
	unsigned int family;
};

///Holds all unowned vulkan handles that are needed to create a vulkan surface.
struct VulkanContext
{
	VkInstance instance_;
	VkPhysicalDevice physicalDevice_;
	VkDevice device_;
	std::vector<VulkanQueueInfo> queues_;
};

// class VulkanSurfaceContext
// {
// protected:
// 	VulkanContext* vulkanContext_;
// 	VkSurfaceKHR surface_;
// };

}
