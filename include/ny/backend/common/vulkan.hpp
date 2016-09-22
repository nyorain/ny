// #pragma once
//
// #include <ny/include.hpp>
// #include <vulkan/vulkan.h>
// #include <vector>
//
// namespace ny
// {
//
// struct VulkanQueueInfo
// {
// 	VkQueue queue;
// 	unsigned int family;
// };
//
// class VulkanContext
// {
// public:
// 	VulkanContext();
// 	~VulkanContext();
//
// protected:
// 	VkInstance instance_;
// 	VkPhysicalDevice physicalDevice_;
// 	VkDevice device_;
// 	std::vector<VulkanQueueInfo> queues_;
// };
//
// class VulkanSurfaceContext
// {
// protected:
// 	VulkanContext* vulkanContext_;
// 	VkSurfaceKHR surface_;
// };
//
// }
