#pragma once

#include <ny/include.hpp>
#include <ny/base/imageData.hpp>

//TODO: documentation.

namespace ny
{

class BufferSurface;

///Manages a drawable buffer in form of an ImageData object.
///When this Guard object is destructed, it will apply the buffer it holds.
class BufferGuard
{
public:
	BufferGuard(BufferSurface&);
	~BufferGuard();

	///Returns a mutable image data buffer to draw into. When this guard get destructed, the
	///buffer it holds will be applied to the associated surface.
	///Note that the format of this image data buffer may vary on different backends and must
	///be taken into account to achieve correct color values.
	///\sa BasicImageData
	///\sa convertFormat
	///\sa BufferSurface
	const MutableImageData& get() { return data_; }

protected:
	MutableImageData data_;
	BufferSurface& surface_;
};

///Software drawable buffer manager. Abstract base class for backends to implement.
class BufferSurface
{
public:
	BufferSurface() = default;
	virtual ~BufferSurface() = default;

	BufferGuard get() { return BufferGuard(*this); }

protected:
	virtual MutableImageData init() = 0;
	virtual void apply(MutableImageData&) = 0;

	friend class BufferGuard;
};

///Holds the different types of draw connection a surface can have.
enum class SurfaceType : unsigned int
{
	none = 0,
	buffer,
	gl,
	vulkan
};

/// Holds the type of draw integration for a WindowContext.
/** ```
auto surface = ny::surface(windowContext);
if(surface.type == ny::SurfaceType::buffer)
{
	//Render into a raw color buffer.
	//e.g. set the first color value of the first pixel to 255.
	auto bufferGuard = surface->buffer->get();
	bufferGuard.get().data[0] = 255;
}
else if(surface.type == ny::SurfaceType::gl)
{
	//Make the gl context current and render using opengl
	auto glContext = surface.gl->makeCurrent();
	glClearColor(1.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
}
else if(surface.type == ny::SurfaceType::vulkan)
{
	//Use a swapchain to render on the given VkSurfaceKHR
	auto swapChain = createSwapChain(surface.vulkan->surface);
	renderOnSwapChain(swapChain);
}
``` **/
class Surface
{
public:
	using Type = SurfaceType;

public:
	Type type = Type::none;
	union
	{
		GlContext* gl {};
		VkSurfaceKHR vulkan;
		std::unique_ptr<BufferSurface> buffer;
	};

public:
	Surface();
	~Surface();

	Surface(Surface&& other) noexcept;
	Surface& operator=(Surface&& other) noexcept;
};

///Returns a surface for the given windowContext, if the associated backend does provide
///an implementation. Otherwise an empty Surface object (with type = Type::none) is returned.
Surface surface(WindowContext&);

}
