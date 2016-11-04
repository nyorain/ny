#pragma once

#include <ny/winapi/include.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/common/cairo.hpp>
#include <nytl/vec.hpp>

namespace ny
{

///X11 implementation for CairoIntegration.
class WinapiCairoIntegration : public WinapiDrawIntegration, public CairoIntegration
{
public:
	WinapiCairoIntegration(WinapiWindowContext&);
	virtual ~WinapiCairoIntegration();

protected:
	//CairoIntegration
	CairoSurfaceGuard get() override;
	void apply(cairo_surface_t&) override;
};

}
