#pragma once

#include <ny/x11/include.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/surface.hpp>

#include <nytl/vec.hpp>
#include <memory>

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
	ImageDataFormat format_ {};
	nytl::Vec2ui size_;
	unsigned int byteSize_ {}; //the size in bytes of (shm_) ? shmaddr_ : data_
	std::uint32_t gc_ {};
	bool shm_ {};

	//when using shm
	unsigned int shmid_ {};
	std::uint32_t shmseg_ {};
	std::uint8_t* shmaddr_ {};

	//otherwise hold owned data
	std::unique_ptr<std::uint8_t[]> data_;
};

}
