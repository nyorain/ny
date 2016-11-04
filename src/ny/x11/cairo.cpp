#include <ny/backend/x11/cairo.hpp>
#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/base/log.hpp>

#include <xcb/xcb.h>
#include <cairo/cairo-xcb.h>

namespace ny
{

//backend/integration/cairo.cpp - private function
using CairoIntegrateFunc = std::function<std::unique_ptr<CairoIntegration>(WindowContext&)>;
unsigned int registerCairoIntegrateFunc(const CairoIntegrateFunc& func);

namespace
{
	std::unique_ptr<CairoIntegration> x11CairoIntegrateFunc(WindowContext& windowContext)
	{
		auto* xwc = dynamic_cast<X11WindowContext*>(&windowContext);
		if(!xwc) return nullptr;
		return std::make_unique<X11CairoIntegration>(*xwc);
	}

	static int registered = registerCairoIntegrateFunc(x11CairoIntegrateFunc);
}

X11CairoIntegration::X11CairoIntegration(X11WindowContext& wc)
	: X11DrawIntegration(wc)
{
	auto size = wc.size();
	auto conn = wc.xConnection();
	surface_ = cairo_xcb_surface_create(conn, wc.xWindow(), wc.xVisualType(), size.x, size.y);
}

X11CairoIntegration::~X11CairoIntegration()
{
	if(surface_) cairo_surface_destroy(surface_);
}

CairoSurfaceGuard X11CairoIntegration::get()
{
	return {*this, *surface_, windowContext_.size()};
}
void X11CairoIntegration::apply(cairo_surface_t&)
{
	cairo_surface_flush(surface_);
}

void X11CairoIntegration::resize(const nytl::Vec2ui& newSize)
{
	cairo_xcb_surface_set_size(surface_, newSize.x, newSize.y);
}

}
