// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/x11/include.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/bufferSurface.hpp>

#include <nytl/vec.hpp>
#include <nytl/nonCopyable.hpp>

#include <memory>

struct xcb_image_t;

namespace ny {

/// X11 BufferSurface implementation.
class X11BufferSurface : public nytl::NonMovable, public BufferSurface {
public:
	X11BufferSurface(X11WindowContext&);
	~X11BufferSurface();

	BufferGuard buffer() override;

	X11WindowContext& windowContext() const { return *windowContext_; }
	xcb_connection_t& xConnection() const { return windowContext().xConnection(); }
	ImageFormat format() const { return format_; }
	bool shm() const { return shm_; }
	bool active() const { return active_; }

protected:
	void apply(const BufferGuard&) noexcept override;
	void resize(nytl::Vec2ui size);

protected:
	X11WindowContext* windowContext_ {};

	ImageFormat format_ {};
	uint32_t gc_ {};
	bool shm_ {};

	bool active_ {};
	nytl::Vec2ui size_; // size of active
	unsigned int byteSize_ {}; // the size in bytes of ((shm_) ? shmaddr_ : data_)
	std::byte* data_ {}; // the actual data (either shmaddr or points so ownedBuffer)

 	// when using shm
	unsigned int shmid_ {};
	uint32_t shmseg_ {};

	// otherwise when using owned buffer because shm not available
	std::unique_ptr<std::byte[]> ownedBuffer_;
};

/// X11 WindowContext implementation with a drawable buffer surface.
class X11BufferWindowContext : public X11WindowContext {
public:
	X11BufferWindowContext(X11AppContext& ac, const X11WindowSettings& settings = {});
	~X11BufferWindowContext() = default;

	Surface surface() override;

protected:
	X11BufferSurface bufferSurface_;
};

} // namespace ny
