// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <cstdint> // std::uintptr_t

namespace ny {

/// Holds the different types of draw integration a surface can have.
/// Used to identify which member of the union in ny::Surface is active.
enum class SurfaceType : unsigned int {
	none = 0,
	buffer,
	gl,
	vulkan
};

/// Holds the type of draw integration for a WindowContext.
/// Can be used to get the associated hardware acceleration surface or a raw
/// memory image buffer into which can be drawn to display context on the
/// associated Window.
/// Note that vulkan surface are stored as uintptr (and can be casted to
/// VkSurfaceKHR if not null).
/// Make sure to correctly implement the surfaceDestroyed and surfaceCreated callback
/// in your window listener to be notified when the surface must no longer be used.
/// Example:
/// ```cpp
/// auto surface = ny::surface(windowContext);
/// if(surface.type == ny::SurfaceType::buffer) {
/// 	// Render into a raw color buffer.
/// 	// e.g. set the first color value of the first pixel to 255.
/// 	auto bufferGuard = surface->buffer->get();
/// 	bufferGuard.get().data[0] = 255;
/// } else if(surface.type == ny::SurfaceType::gl) {
/// 	// Make the gl context current and render using opengl
/// 	auto glSurface = surface.gl;
/// 	glContext->makeCurrent(*glSurface);
/// 	glClearColor(1.f, 0.f, 0.f, 1.f);
/// 	glClear(GL_COLOR_BUFFER_BIT);
/// } else if(surface.type == ny::SurfaceType::vulkan) {
/// 	// Use a swapchain to render on the given VkSurfaceKHR
/// 	auto swapChain = createSwapChain(reinterpret_cast<VkSurfaeKHR>(surface.vulkan->surface));
/// 	renderOnSwapChain(swapChain);
/// }
/// ```
class Surface {
public:
	using Type = SurfaceType;

public:
	Type type = Type::none;
	union
	{
		GlSurface* gl {};
		std::uintptr_t vulkan; // a VkSurfaceKHR
		BufferSurface* buffer;
	};

public:
	Surface() = default;
	Surface(GlSurface& xgl) : type(Type::gl), gl(&xgl) {}
	Surface(std::uintptr_t vk) : type(Type::vulkan), vulkan(vk) {}
	Surface(BufferSurface& bs) : type(Type::buffer), buffer(&bs) {}
	~Surface() = default;

	Surface(const Surface& other) noexcept = default;
	Surface& operator=(const Surface& other) noexcept = default;
};

} // namespace ny
