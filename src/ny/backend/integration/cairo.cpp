#include <ny/backend/integration/cairo.hpp>
#include <vector>

namespace ny
{

//backend/integration/cairo.cpp - private functions interface
using CairoIntegrateFunc = std::function<std::unique_ptr<CairoIntegration>(WindowContext& context)>;
unsigned int registerCairoIntegrateFunc(const CairoIntegrateFunc& func);

//CairoSurfaceGuard
CairoSurfaceGuard::CairoSurfaceGuard(CairoIntegration& integration)
	: surface_(&integration.init()), integration_(&integration)
{
}

CairoSurfaceGuard::~CairoSurfaceGuard()
{
	if(integration_) integration_->apply(*surface_);
}

cairo_surface_t& CairoSurfaceGuard::surface() const
{
	return *surface_;
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
