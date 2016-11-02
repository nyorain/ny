#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/util.hpp>
#include <ny/backend/integration/cairo.hpp>
#include <nytl/vec.hpp>

namespace ny
{

///Wayland implementation for CairoIntegration
class WaylandCairoIntegration : public WaylandDrawIntegration, public CairoIntegration
{
public:
	WaylandCairoIntegration(WaylandWindowContext&);
	virtual ~WaylandCairoIntegration();

protected:
	//CairoIntegration
	///Will select/create an unused buffer and return a surface to draw into it.
	///Does assure that the buffer/surface has the right size.
	CairoSurfaceGuard get() override;

	///Will apply the contents of the surface to the underlaying buffer, attach the buffer
	///to the associated surface and commit it.
	void apply(cairo_surface_t&) override;

	//WaylandDrawIntegration
	///Will resize all unused buffers.
	void resize(const nytl::Vec2ui&) override;

protected:
	struct Buffer 
	{
		wayland::ShmBuffer buffer;
		cairo_surface_t* surface {};
	};

	std::vector<Buffer> buffers_;
	Buffer* active_ {};
};

}
