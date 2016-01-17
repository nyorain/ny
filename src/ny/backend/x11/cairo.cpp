#include <ny/backend/x11/cairo.hpp>

#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/windowContext.hpp>

#include <ny/draw/cairo.hpp>
#include <ny/window/window.hpp>

#include <cairo/cairo-xlib.h>

namespace ny
{

//x11CairoContext
X11CairoDrawContext::X11CairoDrawContext(X11WindowContext& wc)
{
	auto size = wc.window().size();

    xlibSurface_ = cairo_xlib_surface_create(xDisplay(), wc.xWindow(), wc.xVinfo()->visual, 
			size.x, size.y);
    cairoSurface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.x, size.y);
    cairoCR_ = cairo_create(cairoSurface_);
}

X11CairoDrawContext::~X11CairoDrawContext()
{
    cairo_surface_destroy(xlibSurface_);
    cairo_surface_destroy(cairoSurface_);
}

void X11CairoDrawContext::size(const vec2ui& size)
{
    cairo_surface_destroy(cairoSurface_);
    cairo_destroy(cairoCR_);

    cairoSurface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.x, size.y);
    cairoCR_ = cairo_create(cairoSurface_);

    cairo_xlib_surface_set_size(xlibSurface_, size.x, size.y);
}

void X11CairoDrawContext::apply()
{
    cairo_surface_flush(cairoSurface_);
    cairo_surface_show_page(cairoSurface_);

    cairo_t* cr = cairo_create(xlibSurface_);

    cairo_set_source_surface(cr, cairoSurface_, 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr);
}

}
