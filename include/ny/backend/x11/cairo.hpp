#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/integration/cairo.hpp>
#include <nytl/vec.hpp>

namespace ny
{

///X11 implementation for CairoIntegration.
class X11CairoIntegration : public X11DrawIntegration, public CairoIntegration
{
public:
	X11CairoIntegration(X11WindowContext& windowContext);
	virtual ~X11CairoIntegration();

protected:
	//CairoIntegration
	cairo_surface_t& init() override;
	void apply(cairo_surface_t& surf) override;

	//X11DrawIntegration
	void resize(const nytl::Vec2ui& newSize) override;

protected:
	cairo_surface_t* surface_;
};

}
