#include <ny/wayland/cairo.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/interfaces.hpp>

#include <ny/log.hpp>
#include <ny/imageData.hpp>

#include <nytl/rect.hpp>
#include <nytl/vecOps.hpp>

#include <cairo/cairo.h>
#include <wayland-client-protocol.h>

#include <stdexcept>


namespace ny
{

//backend/integration/cairo.cpp - private function
using CairoIntegrateFunc = std::function<std::unique_ptr<CairoIntegration>(WindowContext& context)>;
unsigned int registerCairoIntegrateFunc(const CairoIntegrateFunc& func);

namespace 
{ 
	std::unique_ptr<CairoIntegration> waylandCairoIntegrateFunc(WindowContext& windowContext)
	{
		auto* xwc = dynamic_cast<WaylandWindowContext*>(&windowContext);
		if(!xwc) return nullptr;
		return std::make_unique<WaylandCairoIntegration>(*xwc);
	}

	static int registered = registerCairoIntegrateFunc(waylandCairoIntegrateFunc); 
}

WaylandCairoIntegration::WaylandCairoIntegration(WaylandWindowContext& wc)
	: WaylandDrawIntegration(wc)
{
	//create 2 buffers to begin with?
}

WaylandCairoIntegration::~WaylandCairoIntegration()
{
	for(auto& b : buffers_) if(b.surface) cairo_surface_destroy(b.surface);
	buffers_.clear();
}

CairoSurfaceGuard WaylandCairoIntegration::get()
{
	if(active_)
		throw std::logic_error("WlCairoIntegration: there is already an active SurfaceGuard");

	auto size = windowContext_.size();
	for(auto& b : buffers_)
	{
		if(b.buffer.used()) continue;

		if(!nytl::allEqual(b.buffer.size(), size))
		{
			b.buffer.size(size);
			cairo_surface_destroy(b.surface);
			b.surface = cairo_image_surface_create_for_data(&b.buffer.data(),
				CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);
		}

		active_ = &b;
		b.buffer.use();
		return {*this, *b.surface, size};
	}

	//create new buffer if none is unused
	buffers_.emplace_back();
	buffers_.back().buffer = {windowContext_.appContext(), windowContext_.size()};
	buffers_.back().surface = cairo_image_surface_create_for_data(&buffers_.back().buffer.data(),
		CAIRO_FORMAT_ARGB32, size.x, size.y, size.x * 4);

	buffers_.back().buffer.use();
	active_ = &buffers_.back();
	return {*this, *buffers_.back().surface, size};
}

void WaylandCairoIntegration::apply(cairo_surface_t&)
{
	windowContext_.attachCommit(&active_->buffer.wlBuffer());
	active_ = nullptr;
}

void WaylandCairoIntegration::resize(const nytl::Vec2ui& newSize)
{
	for(auto& b : buffers_)
	{
		if(!b.buffer.used() && !nytl::allEqual(b.buffer.size(), newSize))
		{
			b.buffer.size(windowContext_.size());
			cairo_surface_destroy(b.surface);
			b.surface = cairo_image_surface_create_for_data(&b.buffer.data(),
				CAIRO_FORMAT_ARGB32, newSize.x, newSize.y, newSize.x * 4);
		}
	}
}

}
