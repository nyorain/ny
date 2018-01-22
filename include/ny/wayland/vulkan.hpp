// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/wayland/include.hpp>
#include <ny/wayland/windowContext.hpp> // ny::WaylandWindowContext

#ifndef NY_WithVulkan
	#error ny was built without vulkan. Do not include this header.
#endif

#include <memory> // std::unique_ptr
#include <cstdint> // std::uintptr_t

namespace ny {

/// Wayland WindowContext implementation that also creates a VkSurfaceKHR.
class WaylandVulkanWindowContext : public WaylandWindowContext {
public:
	WaylandVulkanWindowContext(WaylandAppContext&, const WaylandWindowSettings&);
	~WaylandVulkanWindowContext();

	Surface surface() override;

	VkInstance vkInstance() const { return vkInstance_; }
	std::uintptr_t vkSurface() const { return vkSurface_; }

protected:
	VkInstance vkInstance_ {};
	std::uintptr_t vkSurface_ {};
	std::unique_ptr<VkAllocationCallbacks> allocationCallbacks_ {};
};

} // namespace ny
