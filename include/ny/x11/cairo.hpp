#pragma once

#include <ny/x11/include.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/common/cairo.hpp>
#include <nytl/vec.hpp>

namespace ny
{

///X11 implementation for CairoIntegration.
class X11CairoIntegration : public X11DrawIntegration, public CairoIntegration
{
public:
	X11CairoIntegration(X11WindowContext&);
	virtual ~X11CairoIntegration();

protected:
	//CairoIntegration
	CairoSurfaceGuard get() override;
	void apply(cairo_surface_t&) override;

	//X11DrawIntegration
	void resize(const nytl::Vec2ui&) override;

protected:
	cairo_surface_t* surface_;
};

}
