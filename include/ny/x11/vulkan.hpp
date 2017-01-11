// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/x11/include.hpp>
#include <ny/x11/windowContext.hpp>

#ifndef NY_WithVulkan
	#error ny was built without vulkan. Do not include this header.
#endif

namespace ny {

/// X11 WindowContext implementation that also creates a VkSurfaceKHR.
class X11VulkanWindowContext : public X11WindowContext {
public:
	X11VulkanWindowContext(X11AppContext&, const X11WindowSettings&);
	~X11VulkanWindowContext();

	Surface surface() override;

	VkInstance vkInstance() const { return vkInstance_; }
	std::uintptr_t vkSurface() const { return vkSurface_; }

protected:
	VkInstance vkInstance_ {};
	std::uintptr_t vkSurface_ {};
	std::unique_ptr<VkAllocationCallbacks> allocationCallbacks_ {};
};

} // namespace ny
