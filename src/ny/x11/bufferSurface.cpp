// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/bufferSurface.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/log.hpp>
#include <nytl/vecOps.hpp>

#include <xcb/xcb_image.h>
#include <xcb/shm.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <cstring>

namespace ny
{

X11BufferSurface::X11BufferSurface(X11WindowContext& wc) : windowContext_(&wc)
{
	format_ = visualToFormat(*wc.xVisualType(), wc.visualDepth());
	if(format_ == ImageDataFormat::none)
		throw std::runtime_error("ny::X11BufferSurface: Invalid visual format");

	gc_ = xcb_generate_id(&xConnection());
	std::uint32_t value[] = {0, 0};
	auto c = xcb_create_gc_checked(&xConnection(), gc_, wc.xWindow(), XCB_GC_FOREGROUND, value);
	windowContext().errorCategory().checkThrow(c, "ny::X11BufferSurface: create_gc");

	//check if the server has shm suport
	//it is also implemented without shm but the performance might be worse
    auto cookie = xcb_shm_query_version(&xConnection());
    auto reply = xcb_shm_query_version_reply(&xConnection(), cookie, nullptr);

	shm_ = (reply && reply->shared_pixmaps);
	if(reply) free(reply);

	if(!shm_) warning("ny::X11BufferSurface: shm server does not support shm extension");
}

X11BufferSurface::~X11BufferSurface()
{
	if(active_) warning("ny::~X11BufferSurface: there is still an active BufferGuard");
	if(gc_) xcb_free_gc(&xConnection(), gc_);

	if(shmseg_)
	{
		xcb_shm_detach(&xConnection(), shmseg_);
		shmdt(shmaddr_);
		shmctl(shmid_, IPC_RMID, 0);
	}
}

BufferGuard X11BufferSurface::buffer()
{
	if(active_)
		throw std::logic_error("ny::X11BufferSurface::buffer: there is already a BufferGuard");

	//check if resize is needed
	auto size = windowContext().size();
	if(!nytl::allEqual(size, size_))
	{
		// auto newBytes = size.x * size.y * (windowContext().visualDepth() / 8);
		auto newBytes = size.x * size.y * 4;
		size_ = size;

		//we alloc more than is really needed (newBytes * 2) because this will
		//speed up (especially the shm version) resizes.
		if(shm_ && newBytes > byteSize_)
		{
			if(shmseg_)
			{
				xcb_shm_detach(&xConnection(), shmseg_);
				shmdt(shmaddr_);
				shmctl(shmid_, IPC_RMID, 0);
			}
			else
			{
				shmseg_ = xcb_generate_id(&xConnection());
			}

			byteSize_ = newBytes * 2;

			shmid_ = shmget(IPC_PRIVATE, byteSize_, IPC_CREAT | 0777);
			shmaddr_ = static_cast<std::uint8_t*>(shmat(shmid_, 0, 0));
			shmseg_ = xcb_generate_id(&xConnection());
			xcb_shm_attach(&xConnection(), shmseg_, shmid_, 0);
		}
		else if(newBytes > byteSize_)
		{
			byteSize_ = newBytes * 2;
			data_ = std::make_unique<std::uint8_t[]>(byteSize_);
		}
	}

	active_ = true;
	auto stride = size_.x * (windowContext().visualDepth() / 8);
	auto data = data_.get();
	if(shm_) data = shmaddr_;
	return {*this, {data, {size_.x, size_.y}, format_, stride}};
}

void X11BufferSurface::apply(const BufferGuard&) noexcept
{
	if(!active_)
	{
		warning("ny::X11BufferSurface::apply: no currently active BufferGuard");
		return;
	}

	active_ = false;

	auto depth = windowContext().visualDepth();
	auto window = windowContext().xWindow();
	if(shm_)
	{
		xcb_shm_put_image(&xConnection(), window, gc_, size_.x, size_.y, 0, 0, size_.x, size_.y,
			0, 0, depth, XCB_IMAGE_FORMAT_Z_PIXMAP, 0, shmseg_, 0);
	}
	else
	{
		// auto length = size_.x * size_.y * (depth / 8);
		debug("d: ", depth);
		auto length = size_.x * size_.y * 3;
		auto c = xcb_put_image_checked(&xConnection(), XCB_IMAGE_FORMAT_Z_PIXMAP, window, gc_,
			size_.x, size_.y, 0, 0, 0, depth, length, data_.get());
		windowContext().errorCategory().checkWarn(c, "put_image");
	}
}

//X11BufferWindowContext
X11BufferWindowContext::X11BufferWindowContext(X11AppContext& ac, const X11WindowSettings& settings)
	: X11WindowContext(ac, settings), bufferSurface_(*this)
{
	if(settings.buffer.storeSurface) *settings.buffer.storeSurface = &bufferSurface_;
}

Surface X11BufferWindowContext::surface()
{
	return {bufferSurface_};
}

}
