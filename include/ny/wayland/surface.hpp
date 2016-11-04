#pragma once

#include <ny/wayland/include.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/surface.hpp>
#include <nytl/vec.hpp>

namespace ny
{

///Wayland BufferSurface implementation.
class WaylandBufferSurface : public WaylandDrawIntegration, public BufferSurface
{
public:
	WaylandBufferSurface(WaylandWindowContext&);
	~WaylandBufferSurface();

protected:
	MutableImageData init() override;
	void apply(MutableImageData&) override;
	void resize(const nytl::Vec2ui&) override;

protected:
	std::vector<wayland::ShmBuffer> buffers_;
	wayland::ShmBuffer* active_ {};
};

}
