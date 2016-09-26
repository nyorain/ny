#pragma once

#include <ny/include.hpp>
#include <vulkan/vulkan.h>

namespace ny
{

///Holds all unowned vulkan handles that are needed to create a vulkan surface.
struct VulkanContext
{
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
};

///Holds a vulkan surface and a pointer to the context (i.e. instance, physicalDevice and device)
///that were used to create that surface.
struct VulkanSurfaceContext
{
	VulkanContext* vulkanContext;
	VkSurfaceKHR surface;
};

}

#ifndef NY_WithVulkan
	#error ny was built without vulkan. Do not include this header.
#endif
