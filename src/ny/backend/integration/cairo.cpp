#include <ny/backend/integration/cairo.hpp>
#include <vector>

namespace ny
{

//backend/integration/cairo.cpp - private functions interface
using CairoIntegrateFunc = std::function<std::unique_ptr<CairoIntegration>(WindowContext&)>;
unsigned int registerCairoIntegrateFunc(const CairoIntegrateFunc& func);

//CairoSurfaceGuard
CairoSurfaceGuard::CairoSurfaceGuard(CairoIntegration& integration, 
	cairo_surface_t& surf, nytl::Vec2ui size)
	:  integration_(&integration), surface_(&surf), size_(size)
{
}

CairoSurfaceGuard::~CairoSurfaceGuard()
{
	if(integration_) integration_->apply(*surface_);
}

//Cairo integration mechanism
std::vector<CairoIntegrateFunc>& cairoIntegrateFuncs()
{
	static std::vector<CairoIntegrateFunc> funcs;
	return funcs;
}

std::unique_ptr<CairoIntegration> cairoIntegration(WindowContext& context)
{
	for(auto& f : cairoIntegrateFuncs())
	{
		auto ret = f(context);
		if(ret) return ret;
	}

	return nullptr;
}

unsigned int registerCairoIntegrateFunc(const CairoIntegrateFunc& func)
{
	cairoIntegrateFuncs().push_back(func);
	return cairoIntegrateFuncs().size();
}

}
