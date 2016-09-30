#include <ny/backend/x11/surface.hpp>
#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/util.hpp>
#include <ny/base/log.hpp>

#include <xcb/xcb_image.h>
#include <xcb/shm.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <cstring>

namespace ny
{

//backend/integration/surface.cpp - private interface
using SurfaceIntegrateFunc = std::function<Surface(WindowContext&)>;
unsigned int registerSurfaceIntegrateFunc(const SurfaceIntegrateFunc& func);

namespace
{
	Surface x11SurfaceIntegrateFunc(WindowContext& windowContext)
	{
		auto* xwc = dynamic_cast<X11WindowContext*>(&windowContext);
		if(!xwc) return {};

		Surface surface;
		xwc->surface(surface);
		return surface;
	}

	static int registered = registerSurfaceIntegrateFunc(x11SurfaceIntegrateFunc);
}

X11BufferSurface::X11BufferSurface(X11WindowContext& wc) : X11DrawIntegration(wc)
{
	format_ = visualToFormat(*wc.xVisualType(), wc.visualDepth());

	//create the gc
	gc_ = xcb_generate_id(wc.xConnection());
	std::uint32_t value[] = {windowContext_.appContext().xDefaultScreen()->black_pixel};
	// std::uint32_t value[] = {0, 0};
	auto c = xcb_create_gc_checked(wc.xConnection(), gc_, wc.xWindow(), XCB_GC_FOREGROUND, value);
	x11::testCookie(*wc.xConnection(), c, "X11BufferSurface create_gc");

	//check if the server has shm suport
    auto cookie = xcb_shm_query_version(wc.xConnection());
    auto reply = xcb_shm_query_version_reply(wc.xConnection(), cookie, nullptr);

	shm_ = (reply && reply->shared_pixmaps);
	// shm_ = false;
	if(!shm_) warning("X11BufferSurface: xserver has no shm support.");

	resize(wc.size());
}

X11BufferSurface::~X11BufferSurface()
{
	if(shmseg_) 
	{
		xcb_shm_detach(windowContext_.xConnection(), shmseg_);
		shmdt(shmaddr_);
		shmctl(shmid_, IPC_RMID, 0);
	}
}

MutableImageData X11BufferSurface::init()
{
	auto data = data_.get();
	if(shm_) data = shmaddr_;
	return {data, {size_.x, size_.y}, format_, size_.x * (windowContext_.visualDepth() / 8)};
}

void X11BufferSurface::apply(MutableImageData&)
{
	// auto conn = windowContext_.xConnection();
	// auto gc = xcb_generate_id(conn);
	// std::uint32_t value[] = {windowContext_.appContext().xDefaultScreen()->black_pixel};
	// xcb_create_gc(conn, gc, windowContext_.xWindow(), XCB_GC_FOREGROUND, value);
	// // xcb_image_put(conn, windowContext_.xWindow(), gc, image_, 0, 0, 0);
 
	// // auto cookie = xcb_put_image(conn, XCB_IMAGE_FORMAT_Z_PIXMAP, windowContext_.xWindow(), gc, 
	// // 	image_->width, image_->height, 0, 0, 0, 32, image_->width * image_->height * 4, image_->data);
	// // auto error = xcb_request_check(conn, cookie);
	// // debug("error: ", error);
	// // free(error);
	// // xcb_free_gc(conn, gc);
 
	// //TODO: can be done more efficiently with shm
	// //requires some work though and checking for shm extension and xcb_shm
	// // xcb_image_shm_put(windowContext_.xConnection(), windowContext_.xWindow(), 0, image_, )
 	
	// xcb_shm_segment_info_t info;
	// info.shmid   = shmget(IPC_PRIVATE, image_->width * image_->height * 4, IPC_CREAT | 0777);
    // info.shmaddr = static_cast<std::uint8_t*>(shmat(info.shmid, 0, 0));
	// info.shmseg = xcb_generate_id(conn);
    // xcb_shm_attach(conn, info.shmseg, info.shmid, 0);
    // shmctl(info.shmid, IPC_RMID, 0);
	// std::memset(info.shmaddr, 128, image_->width * image_->height * 4);
	// xcb_shm_put_image_checked(conn, windowContext_.xWindow(), gc, image_->width, image_->height, 0, 0,
	// 	image_->width, image_->height, 0, 0, 32, XCB_IMAGE_FORMAT_Z_PIXMAP, 0, info.shmseg, 0);
	// xcb_free_gc(conn, gc);
	// xcb_shm_detach(conn, info.shmseg);
	// shmdt(info.shmaddr);
	// shmctl(info.shmid, IPC_RMID, 0);

	//we are using a pixmap implementation instead of directly drawing on the window drawable
	//since this seems not to work under some circumstances
	auto& wc = windowContext_;
	if(!shm_)
	{
		//if we are not using shm, we first need to copy the local data to the servers pixmap
		auto depth = wc.visualDepth();
		auto c = xcb_put_image_checked(wc.xConnection(), XCB_IMAGE_FORMAT_Z_PIXMAP, pixmap_, gc_, 
			size_.x, size_.y, 0, 0, 0, depth, size_.x * size_.y * (depth / 8), data_.get());
		x11::testCookie(*wc.xConnection(), c, "X11BufferSurface::apply put_image");
	}

	auto c = xcb_copy_area_checked(wc.xConnection(), pixmap_, wc.xWindow(), gc_, 0, 0, 0, 0, 
		size_.x, size_.y);
	x11::testCookie(*wc.xConnection(), c, "X11BufferSurface::apply copy_area");
	xcb_flush(wc.xConnection());
}

void X11BufferSurface::resize(const nytl::Vec2ui& size)
{
	auto& wc = windowContext_;
	auto xconn = wc.xConnection();
	auto depth = wc.visualDepth();
	auto newBytes = size.x * size.y * (depth / 8);
	size_ = size;

	if(pixmap_) xcb_free_pixmap(wc.xConnection(), pixmap_);
	else pixmap_ = xcb_generate_id(wc.xConnection());

	if(shm_)
	{
		if(newBytes > byteSize_)
		{
			if(shmseg_) 
			{
				xcb_shm_detach(xconn, shmseg_);
				shmdt(shmaddr_);
				shmctl(shmid_, IPC_RMID, 0);
			}
			else
			{
				shmseg_ = xcb_generate_id(xconn);
			}

			byteSize_ = newBytes * 2;

			shmid_ = shmget(IPC_PRIVATE, byteSize_, IPC_CREAT | 0777);
			shmaddr_ = static_cast<std::uint8_t*>(shmat(shmid_, 0, 0));
			shmseg_ = xcb_generate_id(xconn);
			xcb_shm_attach(xconn, shmseg_, shmid_, 0);
		}

		auto c = xcb_shm_create_pixmap_checked(xconn, pixmap_, wc.xWindow(), size_.x, size_.y, 
			depth, shmseg_, 0);
		x11::testCookie(*xconn, c, "X11BufferSurface::resize shm_create_pixmap");
	}
	else
	{
		if(newBytes > byteSize_)
		{
			byteSize_ = newBytes * 2;
			data_ = std::make_unique<std::uint8_t[]>(byteSize_);
		}

		auto c = xcb_create_pixmap_checked(xconn, depth, pixmap_, wc.xWindow(), size_.x, size_.y);
		x11::testCookie(*xconn, c, "X11BufferSurface::resize create_pixmap");
	}
}

}
