#pragma once

#include <ny/x11/include.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/bufferSurface.hpp>

#include <nytl/vec.hpp>
#include <nytl/nonCopyable.hpp>

#include <memory>

typedef struct xcb_image_t xcb_image_t;

namespace ny
{

///X11 BufferSurface implementation.
class X11BufferSurface : public BufferSurface
{
public:
	X11BufferSurface() = default;
	X11BufferSurface(X11WindowContext&);
	~X11BufferSurface();

	X11BufferSurface(X11BufferSurface&&) noexcept;
	X11BufferSurface& operator=(X11BufferSurface&&) noexcept;

	BufferGuard buffer() override;
	void apply(const BufferGuard&) noexcept override;

protected:
	X11WindowContext* windowContext_ {};
	ImageDataFormat format_ {};
	nytl::Vec2ui size_;
	unsigned int byteSize_ {}; //the size in bytes of (shm_) ? shmaddr_ : data_
	uint32_t gc_ {};
	bool shm_ {};

	//when using shm
	unsigned int shmid_ {};
	uint32_t shmseg_ {};
	uint8_t* shmaddr_ {};

	//otherwise hold owned data
	std::unique_ptr<std::uint8_t[]> data_;
};

class X11BufferWindowContext : public X11WindowContext
{
public:
};

}
