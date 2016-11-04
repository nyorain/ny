#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/integration/surface.hpp>
#include <nytl/vec.hpp>

namespace ny
{

///Winapi BufferSurface implementation.
class WinapiBufferSurface : public WinapiDrawIntegration, public BufferSurface
{
public:
	WinapiBufferSurface(WinapiWindowContext&);
	~WinapiBufferSurface();

protected:
	MutableImageData init() override;
	void apply(MutableImageData&) override;

protected:
	std::unique_ptr<std::uint8_t[]> data_;
	nytl::Vec2ui size_;
};

}
