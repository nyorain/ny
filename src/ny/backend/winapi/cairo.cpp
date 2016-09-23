#include <ny/backend/winapi/cairo.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/base/log.hpp>

#include <cairo/cairo-win32.h>

namespace ny
{

//backend/integration/cairo.cpp - private function
using CairoIntegrateFunc = std::function<std::unique_ptr<CairoIntegration>(WindowContext& context)>;
unsigned int registerCairoIntegrateFunc(const CairoIntegrateFunc& func);

namespace
{
	std::unique_ptr<CairoIntegration> winapiCairoIntegrateFunc(WindowContext& windowContext)
	{
		auto* xwc = dynamic_cast<WinapiWindowContext*>(&windowContext);
		if(!xwc) return nullptr;
		return std::make_unique<WinapiCairoIntegration>(*xwc);
	}

	static int registered = registerCairoIntegrateFunc(winapiCairoIntegrateFunc);
}

WinapiCairoIntegration::WinapiCairoIntegration(WinapiWindowContext& wc)
	: WinapiDrawIntegration(wc)
{
}

WinapiCairoIntegration::~WinapiCairoIntegration()
{
}

cairo_surface_t& WinapiCairoIntegration::init()
{
	auto hdc = ::GetDC(context_.handle());
	auto surface = cairo_win32_surface_create(hdc);
}
void WinapiCairoIntegration::apply(cairo_surface_t& surface)
{
	auto hdc = cairo_win32_surface_get_dc(&surface);
	cairo_surface_flush(&surface);
	cairo_surface_destroy(&surface);
	::ReleaseDC(context_.handle(), hdc);
}

}
