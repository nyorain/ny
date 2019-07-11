// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/bufferSurface.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/util.hpp>
#include <nytl/vecOps.hpp>
#include <dlg/dlg.hpp>

#include <xcb/xcb_image.h>
#include <xcb/shm.h>
#include <xcb/present.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <cstring>

// sources (not including any present protocol stuff, see keithp blog for that):
// https://github.com/freedesktop-unofficial-mirror/xcb__util-image/blob/master/image/xcb_image.c#L158
// http://xcb.pdx.freedesktop.narkive.com/0u3XxxGY/xcb-put-image
// https://github.com/freedesktop-unofficial-mirror/xcb__util-image/blob/master/image/xcb_image.h
// https://www.x.org/releases/X11R7.6/doc/xproto/x11protocol.html#requests:PutImage
// https://tronche.com/gui/x/xlib/graphics/XPutImage.html
// https://keithp.com/blogs/Present/

// Note that this is way harder to implement on x than it should be due to
// possible absence of the shm extension and additional required format
// querying (bpp != depth).  Implementation mainly oriented at xcb-util-image.
// xcb-util-image was not used directly since we want to avoid additional
// overhead at resizing and since using it would make the implementation (shm
// switch) even more complex.

// When not using the shm version, we should have to check not to exceed the
// maximum request length, but since this is usually pretty low (even with big
// requests extensions somewhere around 4MB on test machine) band putting the
// image onto the window works nontheless (somehow?!) and xcb-util-image does
// not check/split as well, so we don't check for request length exceeding.

namespace ny {

X11BufferSurface::X11BufferSurface(X11WindowContext& wc) : windowContext_(&wc) {
	gc_ = xcb_generate_id(&xConnection());
	std::uint32_t value[] = {0, 0};
	auto c = xcb_create_gc_checked(&xConnection(), gc_, wc.xWindow(),
		XCB_GC_FOREGROUND, value);
	windowContext().errorCategory().checkThrow(c,
		"ny::X11BufferSurface: create_gc");

	// query the format
	// this is needed because the xserver may need a different bpp for an image
	// with the depth of the window.
	// For 24-bit depth images the xserver often required 32 bpp.
	auto setup = xcb_get_setup(&xConnection());
	auto fmtit = xcb_setup_pixmap_formats(setup);
	auto fmtend = fmtit + xcb_setup_pixmap_formats_length(setup);
	xcb_format_t* fmt {};

	while(fmtit != fmtend) {
		if(fmtit->depth == windowContext().visualDepth()) {
			fmt = fmtit;
			break;
		}
		++fmtit;
	}

	if(!fmt) {
		auto msg = "ny::X11BufferSurface: couldn't query depth format bpp";
		throw std::runtime_error(msg);
	}

	format_ = x11::visualToFormat(*windowContext().xVisualType(),
		fmt->bits_per_pixel);
	if(format_ == ImageFormat::none) {
		auto msg = "ny::X11BufferSurface: couldn't parse visual format";
		throw std::runtime_error(msg);
	}

	if(windowContext().appContext().shmExt()) {
		if(windowContext().appContext().presentExt()) {
			mode_ = Mode::presentShm;
		} else {
			mode_ = Mode::shm;
		}
	} else {
		mode_ = Mode::dumb;
	}

	mode_ = Mode::dumb;
}

X11BufferSurface::~X11BufferSurface() {
	if(active_) {
		dlg_warn("there is still an active BufferGuard");
	}

	destroyShm(shm_);
	for(auto& pix : pixmaps_) {
		destroyShm(pix.shm);
		if(pix.pixmap) {
			xcb_free_pixmap(&xConnection(), pix.pixmap);
		}
	}

	if(gc_) {
		xcb_free_gc(&xConnection(), gc_);
	}
}

void X11BufferSurface::destroyShm(const Shm& shm) {
	if(shm.seg) {
		xcb_shm_detach(&xConnection(), shm.seg);
		shmdt(shm.data);
		shmctl(shm.id, IPC_RMID, 0);
	}
}

void X11BufferSurface::createShm(Shm& shm, std::size_t size) {
	shm.seg = xcb_generate_id(&xConnection());
	shm.id = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
	shm.data = static_cast<std::byte*>(shmat(shm.id, 0, 0));
	xcb_shm_attach(&xConnection(), shm.seg, shm.id, 0);
}

void X11BufferSurface::release(uint32_t serial) {
	for(auto& p : pixmaps_) {
		if(p.serial == serial) {
			p.serial = {};
			break;
		}
	}
}

BufferGuard X11BufferSurface::buffer() {
	if(active_) {
		auto msg = "ny::X11BufferSurface::buffer: there is already a BufferGuard";
		throw std::logic_error(msg);
	}

	// check if resize is needed
	// NOTE: querySize() might be better here so that the buffer fits the
	// window better. But that might return an Image with a different
	// size than the known size of the application.
	auto size = windowContext().size();
	auto newBytes = std::ceil(size[0] * size[1] * bitSize(format_) / 8.0);
	std::byte* data {};

	if(mode_ == Mode::presentShm) {
		// search for free buffer
		Pixmap* found {};
		for(auto& p : pixmaps_) {
			if(!p.serial) {
				found = &p;
				break;
			}
		}

		if(!found) {
			pixmaps_.emplace_back();
			found = &pixmaps_.back();
		}

		if(found->size.x != size.x || found->size.y != size.y) {
			if(found->pixmap) {
				xcb_free_pixmap(&xConnection(), found->pixmap);
			} else {
				found->pixmap = xcb_generate_id(&xConnection());
			}

			found->size = size;
			createShm(found->shm, newBytes);
			auto cookie = xcb_shm_create_pixmap_checked(&xConnection(),
				found->pixmap, windowContext().xWindow(), size.x, size.y,
				windowContext().visualDepth(), found->shm.seg, 0);
			windowContext().errorCategory().checkWarn(cookie,
				"ny::X11BufferSurface: shm_create_pixmap");
		}

		data = found->shm.data;
		activePixmap_ = found;
	} else if(newBytes > byteSize_) {
		// we alloc more than is really needed because this will speed up
		// (especially the shm version) resizes. We don't have to reallocated
		// every time the window is resized and redrawn for the cost of higher
		// memory consumption
		byteSize_ = newBytes * 4;
		if(mode_ == Mode::shm) {
			destroyShm(shm_);
			createShm(shm_, byteSize_);
			data = shm_.data;
		} else {
			ownedBuffer_ = std::make_unique<std::byte[]>(byteSize_);
			data = ownedBuffer_.get();
		}
	} else {
		data = mode_ == Mode::shm ? shm_.data : ownedBuffer_.get();
	}

	size_ = size;
	active_ = true;

	auto stride = size_[0] * bitSize(format_);
	return {*this, {data, {size_[0], size_[1]}, format_, stride}};
}

void X11BufferSurface::apply(const BufferGuard&) noexcept {
	if(!active_) {
		dlg_warn("no currently active BufferGuard");
		return;
	}

	active_ = false;
	if(mode_ == Mode::presentShm) {
		auto serial = windowContext().presentSerial();
		activePixmap_->serial = serial;
		auto cookie = xcb_present_pixmap_checked(&xConnection(),
			windowContext().xWindow(), activePixmap_->pixmap, serial, 0, 0, 0, 0,
			XCB_NONE, XCB_NONE, XCB_NONE, 0, 0, 0, 0, 0, nullptr);
		windowContext().errorCategory().checkWarn(cookie,
			"ny::X11BufferSurface: present_pixmap");
		activePixmap_ = nullptr;
		return;
	}

	// NOTE: we use the checked versions here since those function are very
	// error prone due to the rather complex depth/visual/bpp x system. We
	// catch invalid x request here directly.  maybe remove this later on if
	// tested enough?
	auto depth = windowContext().visualDepth();
	auto window = windowContext().xWindow();
	if(mode_ == Mode::shm) {
		auto cookie = xcb_shm_put_image_checked(&xConnection(), window, gc_,
			size_[0], size_[1], 0, 0, size_[0], size_[1], 0, 0, depth,
			XCB_IMAGE_FORMAT_Z_PIXMAP, 0, shm_.seg, 0);
		windowContext().errorCategory().checkWarn(cookie,
			"ny::X11BufferSurface: shm_put_image");
	} else {
		auto length = std::ceil(size_[0] * size_[1] * bitSize(format_) / 8.0);
		auto data = reinterpret_cast<uint8_t*>(ownedBuffer_.get());
		auto cookie = xcb_put_image_checked(&xConnection(),
			XCB_IMAGE_FORMAT_Z_PIXMAP, window, gc_, size_[0], size_[1], 0, 0, 0,
			depth, length, data);
		windowContext().errorCategory().checkWarn(cookie,
			"ny::X11BufferSurface: put_image");
	}
}

// X11BufferWindowContext
X11BufferWindowContext::X11BufferWindowContext(X11AppContext& ac,
	const X11WindowSettings& settings) : X11WindowContext(ac, settings),
		bufferSurface_(*this) {
	// TODO: we could implement a custom visual querying here
	// we need to find a visual with a known format
	if(settings.buffer.storeSurface) {
		*settings.buffer.storeSurface = &bufferSurface_;
	}
}

Surface X11BufferWindowContext::surface() {
	return {bufferSurface_};
}

void X11BufferWindowContext::presentCompleteEvent(uint32_t serial) {
	X11WindowContext::presentCompleteEvent(serial);
	bufferSurface_.release(serial);
}

} // namespace ny
