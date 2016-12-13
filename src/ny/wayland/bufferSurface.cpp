// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/bufferSurface.hpp>
#include <ny/wayland/util.hpp>
#include <ny/log.hpp>
#include <ny/surface.hpp>

#include <nytl/vecOps.hpp>
#include <stdexcept>

namespace ny
{

//WaylandBufferSurface
WaylandBufferSurface::WaylandBufferSurface(WaylandWindowContext& wc) : windowContext_(&wc)
{
}

WaylandBufferSurface::~WaylandBufferSurface()
{
	if(active_) warning("ny::~WaylandBufferSurface: there is still an active BufferGuard");
}

BufferGuard WaylandBufferSurface::buffer()
{
	if(active_)
		throw std::logic_error("ny::WlBufferSurface: there is already an active BufferGuard");

	auto size = windowContext().size();
	for(auto& b : buffers_)
	{
		if(b.used()) continue;
		if(!nytl::allEqual(b.size(), size)) b.size(size);

		b.use();
		active_ = &b;
		auto format = waylandToImageFormat(b.format());
		return {*this, {&b.data(), size, format, b.stride()}};
	}

	//create new buffer if none is unused
	buffers_.emplace_back(windowContext().appContext(), size);
	buffers_.back().use();
	active_ = &buffers_.back();
	auto format = waylandToImageFormat(buffers_.back().format());
	if(format == imageFormats::none)
		throw std::runtime_error("ny::WaylandBufferSurface: failed to parse shm buffer format");

	return {*this, {&buffers_.back().data(), size, format, buffers_.back().stride() * 8}};
}

void WaylandBufferSurface::apply(const BufferGuard& buffer) noexcept
{
	if(!active_ || buffer.get().data != &active_->data())
	{
		warning("ny::WaylandBufferSurface::apply: invalid BufferGuard given");
		return;
	}

	if(windowContext().shown()) windowContext().attachCommit(&active_->wlBuffer());
	active_ = nullptr;
}

//WaylandBufferWindowContext
WaylandBufferWindowContext::WaylandBufferWindowContext(WaylandAppContext& ac,
	const WaylandWindowSettings& settings) :
		WaylandWindowContext(ac, settings),
		bufferSurface_(*this)
{
	if(settings.buffer.storeSurface) *settings.buffer.storeSurface = &bufferSurface_;
}

Surface WaylandBufferWindowContext::surface()
{
	return {bufferSurface_};
}

}
