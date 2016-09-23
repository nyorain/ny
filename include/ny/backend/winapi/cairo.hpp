#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/integration/cairo.hpp>
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
	cairo_surface_t& init() override;
	void apply(cairo_surface_t&) override;
};

}
