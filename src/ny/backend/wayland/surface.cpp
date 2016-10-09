#include <ny/backend/wayland/surface.hpp>
#include <ny/backend/wayland/util.hpp>
#include <ny/base/log.hpp>
#include <nytl/vecOps.hpp>

#include <stdexcept>

namespace ny
{

//backend/integration/surface.cpp - private interface
using SurfaceIntegrateFunc = std::function<Surface(WindowContext&)>;
unsigned int registerSurfaceIntegrateFunc(const SurfaceIntegrateFunc& func);

namespace
{
	Surface waylandSurfaceIntegrateFunc(WindowContext& windowContext)
	{
		auto* xwc = dynamic_cast<WaylandWindowContext*>(&windowContext);
		if(!xwc) return {};

		Surface surface;
		xwc->surface(surface);
		return surface;
	}

	static int registered = registerSurfaceIntegrateFunc(waylandSurfaceIntegrateFunc);
}

WaylandBufferSurface::WaylandBufferSurface(WaylandWindowContext& wc) : WaylandDrawIntegration(wc)
{
	//create 2 buffers to begin with?
}

WaylandBufferSurface::~WaylandBufferSurface()
{
}

MutableImageData WaylandBufferSurface::init()
{
	if(active_)
		throw std::logic_error("WlCairoIntegration: there is already an active SurfaceGuard");

	auto size = windowContext_.size();
	for(auto& b : buffers_)
	{
		if(b.used()) continue;
		if(nytl::anyOf(b.size() != size)) b.size(size);

		b.use();
		active_ = &b;
		auto format = waylandToImageFormat(b.format());
		return {&b.data(), size, format, size.x * 4};
	}

	//create new buffer if none is unused
	buffers_.emplace_back(windowContext_.appContext(), windowContext_.size());
	buffers_.back().use();
	active_ = &buffers_.back();
	auto format = waylandToImageFormat(buffers_.back().format());
	if(format == ImageDataFormat::none)
		warning("WaylandBufferSurface: failed to parse format");

	return {&buffers_.back().data(), size, format, size.x * 4};
}

void WaylandBufferSurface::apply(MutableImageData&)
{
	windowContext_.attachCommit(active_->wlBuffer());
	active_ = nullptr;
}

void WaylandBufferSurface::resize(const nytl::Vec2ui& newSize)
{
	for(auto& b : buffers_)
		if(!b.used() && nytl::anyOf(b.size() != newSize))
			b.size(windowContext_.size());
}

}
