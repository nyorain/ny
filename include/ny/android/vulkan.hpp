// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <ny/android/windowContext.hpp>

#ifndef NY_WithVulkan
	#error ny was built without vulkan. Do not include this header.
#endif

#include <memory> // std::unique_ptr
#include <cstdint> // std::uintptr_t

namespace ny {

/// Wayland WindowContext implementation that also creates a VkSurfaceKHR.
class AndroidVulkanWindowContext : public AndroidWindowContext {
public:
	AndroidVulkanWindowContext(AndroidAppContext&, const AndroidWindowSettings&);
	~AndroidVulkanWindowContext();

	Surface surface() override;
	using AndroidWindowContext::nativeWindow;

	VkInstance vkInstance() const { return vkInstance_; }
	std::uintptr_t vkSurface() const { return vkSurface_; }

protected:
	void nativeWindow(ANativeWindow*) override;

protected:
	VkInstance vkInstance_ {};
	std::uintptr_t vkSurface_ {};
	std::unique_ptr<VkAllocationCallbacks> allocCbs_ {};
};

} // namespace ny
