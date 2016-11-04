#include <ny/winapi/cairo.hpp>
#include <ny/winapi/appContext.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/log.hpp>

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

		try { auto ret = std::make_unique<WinapiCairoIntegration>(*xwc); return ret; }
		catch(const std::exception&) {}

		return nullptr;
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

CairoSurfaceGuard WinapiCairoIntegration::get()
{
	auto size = windowContext_.clientExtents().size;
	auto surface = cairo_win32_surface_create_with_dib(CAIRO_FORMAT_ARGB32, size.x, size.y);
	return {*this, *surface, size};
}
void WinapiCairoIntegration::apply(cairo_surface_t& surface)
{
	cairo_surface_flush(&surface);
	auto hdc = cairo_win32_surface_get_dc(&surface);

	auto extents = windowContext_.clientExtents();
	auto whdc = ::GetDC(windowContext_.handle());
	BitBlt(whdc, 0, 0, extents.width(), extents.height(), hdc, 0, 0, SRCCOPY);
	ReleaseDC(windowContext_.handle(), hdc);
	cairo_surface_destroy(&surface);
}

}
