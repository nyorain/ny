#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/integration/surface.hpp>
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
