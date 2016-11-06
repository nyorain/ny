#pragma once

#include <ny/android/include.hpp>
#include <ny/android/windowContext.hpp>
#include <ny/surface.hpp>

#include <nytl/vec.hpp>

namespace ny
{

///Android BufferSurface implementatoin.
class AndroidBufferSurface : public AndroidDrawIntegration, public BufferSurface
{
public:
	AndroidBufferSurface(AndroidWindowContext&);
	~AndroidBufferSurface() = default;

protected:
	MutableImageData init() override;
	void apply(MutableImageData&) override;

protected:
	ANativeWindow_Buffer buffer_;
};

}
