#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/integration/surface.hpp>
#include <nytl/vec.hpp>

typedef struct xcb_image_t xcb_image_t;

namespace ny
{

///X11 BufferSurface implementation.
class X11BufferSurface : public X11DrawIntegration, public BufferSurface
{
public:
	X11BufferSurface(X11WindowContext&);
	~X11BufferSurface();

protected:
	MutableImageData init() override;
	void apply(MutableImageData&) override;
	void resize(const nytl::Vec2ui&) override;

protected:
	xcb_image_t* image_ {};
	ImageDataFormat format_ {};
	unsigned int byteSize_ {};
};

}
