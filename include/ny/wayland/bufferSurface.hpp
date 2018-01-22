// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/wayland/include.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/bufferSurface.hpp>

#include <nytl/vec.hpp>
#include <nytl/nonCopyable.hpp>

#include <vector>

namespace ny {

/// Wayland BufferSurface implementation.
class WaylandBufferSurface : public nytl::NonCopyable, public BufferSurface {
public:
	WaylandBufferSurface(WaylandWindowContext&);
	~WaylandBufferSurface();

	BufferGuard buffer() override;
	void apply(const BufferGuard&) noexcept override;

	WaylandWindowContext& windowContext() const { return *windowContext_; }
	const std::vector<wayland::ShmBuffer>& shmBuffers() const { return buffers_; }
	wayland::ShmBuffer* active() const { return active_; }

protected:
	WaylandWindowContext* windowContext_ {};
	std::vector<wayland::ShmBuffer> buffers_;
	wayland::ShmBuffer* active_ {};
};

/// WaylandWindowContext for a BufferSurface.
class WaylandBufferWindowContext : public WaylandWindowContext {
public:
	WaylandBufferWindowContext(WaylandAppContext&, const WaylandWindowSettings& = {});
	~WaylandBufferWindowContext() = default;

	Surface surface() override;

protected:
	WaylandBufferSurface bufferSurface_;
};

} // namespace ny
