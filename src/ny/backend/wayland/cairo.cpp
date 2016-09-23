#include <ny/backend/wayland/cairo.hpp>
#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/wayland/interfaces.hpp>

#include <ny/base/log.hpp>
#include <ny/base/imageData.hpp>

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
	for(auto& b : buffers_) cairo_surface_destroy(b.surface);
	buffers_.clear();
}

cairo_surface_t& WaylandCairoIntegration::init()
{
	if(active_)
		throw std::logic_error("WlCairoIntegration: there is already an active SurfaceGuard");

	for(auto& b : buffers_)
	{
		if(b.buffer.used()) continue;

		if(nytl::any(b.buffer.size() != context_.size()))
		{
			b.buffer.size(context_.size());
			buffers_.back().surface = cairo_image_surface_create_for_data(&b.buffer.data(),
				CAIRO_FORMAT_ARGB32, context_.size().x, context_.size().y, context_.size().x * 4);
		}

		active_ = &b;
		b.buffer.use();
		return *b.surface;
	}

	//create new buffer if none is unused
	buffers_.emplace_back();
	buffers_.back().buffer = {context_.appContext(), context_.size()};
	buffers_.back().surface = cairo_image_surface_create_for_data(&buffers_.back().buffer.data(),
		CAIRO_FORMAT_ARGB32, context_.size().x, context_.size().y, context_.size().x * 4);

	buffers_.back().buffer.use();
	active_ = &buffers_.back();
	return *buffers_.back().surface;
}

void WaylandCairoIntegration::apply(cairo_surface_t&)
{
	context_.attachCommit(active_->buffer.wlBuffer());
	active_ = nullptr;
}

void WaylandCairoIntegration::resize(const nytl::Vec2ui& newSize)
{
	for(auto& b : buffers_)
	{
		if(!b.buffer.used() && nytl::any(b.buffer.size() != newSize))
		{
			b.buffer.size(context_.size());
			buffers_.back().surface = cairo_image_surface_create_for_data(&b.buffer.data(),
				CAIRO_FORMAT_ARGB32, newSize.x, newSize.y, newSize.x * 4);
		}
	}
}

}
