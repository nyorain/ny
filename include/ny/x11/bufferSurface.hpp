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
	bool active() const { return active_; }
	void release(uint32_t serial);

protected:
	enum class Mode {
		dumb, // simple local buffer
		shm, // shared memory, using xcb_image_put
		presentShm, // shared memory, used with present extension
	};

	struct Shm {
		unsigned int id {};
		uint32_t seg {};
		std::byte* data {};
	};

	struct Pixmap {
		Shm shm {};
		xcb_pixmap_t pixmap {};
		uint32_t serial {};
		nytl::Vec2ui size {};
	};

	void apply(const BufferGuard&) noexcept override;
	void resize(nytl::Vec2ui size);
	void destroyShm(const Shm& shm);
	void createShm(Shm& shm, std::size_t size);

protected:
	X11WindowContext* windowContext_ {};

	ImageFormat format_ {};
	uint32_t gc_ {};

	bool active_ {}; // whether currently active
	Pixmap* activePixmap_ {}; // for presentShm mode: active pixmap
	nytl::Vec2ui size_; // size of active
	Mode mode_;

	Shm shm_; // when using shm
	std::vector<Pixmap> pixmaps_; // when using present shm pixmaps

	// otherwise when using owned buffer because shm not available
	std::unique_ptr<std::byte[]> ownedBuffer_;
	unsigned int byteSize_ {}; // for shm or dumb mode
};

/// X11 WindowContext implementation with a drawable buffer surface.
class X11BufferWindowContext : public X11WindowContext {
public:
	X11BufferWindowContext(X11AppContext& ac, const X11WindowSettings& settings = {});
	~X11BufferWindowContext() = default;

	Surface surface() override;
	void presentCompleteEvent(uint32_t serial) override;

protected:
	X11BufferSurface bufferSurface_;
};

} // namespace ny
