#include <ny/backend/x11/cairo.hpp>
#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/draw/cairo.hpp>
#include <ny/base/log.hpp>
#include <ny/window/events.hpp>

#include <cairo/cairo-xcb.h>

namespace ny
{

X11CairoWindowContext::X11CairoWindowContext(X11AppContext& ctx, const X11WindowSettings& settings)
{
	X11WindowContext::create(ctx, settings);
	auto surface = cairo_xcb_surface_create(xConnection(), xWindow(), visualType_, 
			settings.size.x, settings.size.y);

	drawContext_.reset(new CairoDrawContext(*surface));
	cairo_surface_destroy(surface);
}

X11CairoWindowContext::~X11CairoWindowContext() = default;

void X11CairoWindowContext::initVisual()
{
	xcb_depth_iterator_t depth_iter;

	depth_iter = xcb_screen_allowed_depths_iterator(appContext().xDefaultScreen());
	while(depth_iter.rem)
	{
		xcb_visualtype_iterator_t visual_iter;

		visual_iter = xcb_depth_visuals_iterator (depth_iter.data);
		while(visual_iter.rem) 
		{
			if(appContext().xDefaultScreen()->root_visual == visual_iter.data->visual_id) 
			{
				visualType_ = visual_iter.data;
				break;
			}

			xcb_visualtype_next(&visual_iter);
		} 
		
		xcb_depth_next (&depth_iter);
	}

	xVisualID_ = visualType_->visual_id;
}

DrawGuard X11CairoWindowContext::draw()
{
	cairo_reset_clip(drawContext_->cairoContext());
	return *drawContext_;
}

void X11CairoWindowContext::resizeCairo(const Vec2ui& size)
{
	cairo_xcb_surface_set_size(drawContext_->cairoSurface(), size.x, size.y);
}

void X11CairoWindowContext::size(const Vec2ui& size)
{
	X11WindowContext::size(size);
	resizeCairo(size);
}

bool X11CairoWindowContext::handleEvent(const Event& e)
{
	if(X11WindowContext::handleEvent(e)) return true;

	if(e.type() == eventType::windowSize)
	{
		resizeCairo(static_cast<const SizeEvent&>(e).size);
		return true;
	}

	return false;
}

}
