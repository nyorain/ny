#include <ny/backend/x11/surface.hpp>
#include <ny/backend/x11/util.hpp>

#include <xcb/xcb_image.h>

namespace ny
{

//backend/integration/surface.cpp - private interface
using SurfaceIntegrateFunc = std::function<Surface(WindowContext&)>;
unsigned int registerSurfaceIntegrateFunc(const SurfaceIntegrateFunc& func);

namespace
{
	Surface x11SurfaceIntegrateFunc(WindowContext& windowContext)
	{
		auto* xwc = dynamic_cast<X11WindowContext*>(&windowContext);
		if(!xwc) return {};

		Surface surface;
		xwc->surface(surface);
		return surface;
	}

	static int registered = registerSurfaceIntegrateFunc(x11SurfaceIntegrateFunc);
}

X11BufferSurface::X11BufferSurface(X11WindowContext& wc) : X11DrawIntegration(wc)
{
	format_ = visualToFormat(*wc.xVisualType());
}
X11BufferSurface::~X11BufferSurface()
{
	if(image_) xcb_image_destroy(image_);
}

MutableImageData X11BufferSurface::init()
{
	if(!image_)
	{
		auto size = windowContext_.size();
		byteSize_ = size.x * size.y * 4;
		image_ = xcb_image_create_native(windowContext_.xConnection(), size.x, size.y, 
			XCB_IMAGE_FORMAT_Z_PIXMAP, 32, nullptr, byteSize_, nullptr);
	}

	return {image_->data, {image_->width, image_->height}, format_, image_->width * 4u};
}

void X11BufferSurface::apply(MutableImageData&)
{
	xcb_image_put(windowContext_.xConnection(), windowContext_.xWindow(), 0, image_, 0, 0, 0);

	//TODO: can be done more efficiently with shm
	//requires some work though and checking for shm extension and xcb_shm
	// xcb_image_shm_put(windowContext_.xConnection(), windowContext_.xWindow(), 0, image_, )
}

void X11BufferSurface::resize(const nytl::Vec2ui& size)
{
	image_->width = size.x;
	image_->height = size.y;

	auto newBytes = size.x * size.y * 4;
	if(newBytes > byteSize_)
	{
		byteSize_ = newBytes;
		image_->base = std::malloc(newBytes);
	}
}

}
