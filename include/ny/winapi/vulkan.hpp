// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

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

	VkInstance vkInstance() const { return vkInstance_; }
	std::uintptr_t vkSurface() const { return vkSurface_; }

protected:
	VkInstance vkInstance_ {};
	std::uintptr_t vkSurface_ {};
	std::unique_ptr<VkAllocationCallbacks> allocationCallbacks_ {};
};

}

#ifndef NY_WithVulkan
	#error ny was built without vulkan. Do not include this header.
#endif
