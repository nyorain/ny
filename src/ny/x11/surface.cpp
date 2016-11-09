#include <ny/x11/surface.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/log.hpp>

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
	if(format_ == ImageDataFormat::none)
		throw std::runtime_error("ny::X11BufferSurface: Invalid visual format");

	gc_ = xcb_generate_id(wc.xConnection());
	std::uint32_t value[] = {0, 0};
	auto c = xcb_create_gc_checked(wc.xConnection(), gc_, wc.xWindow(), XCB_GC_FOREGROUND, value);
	windowContext_.errorCategory().checkThrow(c, "ny::X11BufferSurface: create_gc");

	//check if the server has shm suport
	//it is also implemented without shm but the performance might be worse
    auto cookie = xcb_shm_query_version(wc.xConnection());
    auto reply = xcb_shm_query_version_reply(wc.xConnection(), cookie, nullptr);

	shm_ = (reply && reply->shared_pixmaps);
	if(reply) free(reply);

	if(!shm_) warning("ny::X11BufferSurface: shm server does not support shm extension");
	resize(wc.size()); //setup shm/buffer
}

X11BufferSurface::~X11BufferSurface()
{
	if(gc_) xcb_free_gc(windowContext_.xConnection(), gc_);

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
	auto& wc = windowContext_;
	auto depth = wc.visualDepth();
	if(!shm_)
	{
		auto length = size_.x * size_.y * (depth / 8);
		xcb_put_image(wc.xConnection(), XCB_IMAGE_FORMAT_Z_PIXMAP,
			wc.xWindow(), gc_, size_.x, size_.y, 0, 0, 0, depth, length, data_.get());
	}
	else
	{
		xcb_shm_put_image(wc.xConnection(), wc.xWindow(), gc_, size_.x,
			size_.y, 0, 0, size_.x, size_.y, 0, 0, depth, XCB_IMAGE_FORMAT_Z_PIXMAP, 0, shmseg_, 0);
	}
}

void X11BufferSurface::resize(const nytl::Vec2ui& size)
{
	auto& wc = windowContext_;
	auto xconn = wc.xConnection();
	auto depth = wc.visualDepth();
	auto newBytes = size.x * size.y * (depth / 8);
	size_ = size;

	if(shm_ && newBytes > byteSize_)
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
	else if(newBytes > byteSize_)
	{
		byteSize_ = newBytes * 2;
		data_ = std::make_unique<std::uint8_t[]>(byteSize_);
	}
}

}