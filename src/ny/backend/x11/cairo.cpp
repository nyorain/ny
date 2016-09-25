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
	//find the visual type for the windowContexts visual
	xcb_depth_iterator_t depth_iter;
	xcb_visualtype_t* visualtype;

	depth_iter = xcb_screen_allowed_depths_iterator(wc.appContext().xDefaultScreen());
	while(depth_iter.rem)
	{
		xcb_visualtype_iterator_t visual_iter;

		visual_iter = xcb_depth_visuals_iterator (depth_iter.data);
		while(visual_iter.rem)
		{
			if(wc.xVisualID() == visual_iter.data->visual_id)
			{
				visualtype = visual_iter.data;
				break;
			}

			xcb_visualtype_next(&visual_iter);
		}

		xcb_depth_next (&depth_iter);
	}

	//query the size of the window
	auto cookie = xcb_get_geometry(wc.xConnection(), wc.xWindow());
	auto geometry = xcb_get_geometry_reply(wc.xConnection(), cookie, nullptr);

	surface_ = cairo_xcb_surface_create(wc.xConnection(), wc.xWindow(), visualtype,
			geometry->width, geometry->height);
	free(geometry);
}

X11CairoIntegration::~X11CairoIntegration()
{
	if(surface_) cairo_surface_destroy(surface_);
}

cairo_surface_t& X11CairoIntegration::init()
{
	return *surface_;
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
