#include <ny/wayland/util.hpp>

#include <ny/wayland/appContext.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/cairo.hpp>

#include <ny/event.hpp>
#include <ny/log.hpp>
#include <ny/cursor.hpp>
#include <ny/imageData.hpp>

#include <nytl/scope.hpp>

#include <wayland-client-protocol.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <linux/input.h>
#include <iostream>

namespace ny
{

namespace wayland
{

//utility functions
namespace
{

int setCloexecOrClose(int fd)
{
    long flags;

    if(fd == -1)
        return -1;

    flags = fcntl(fd, F_GETFD);
    if(flags == -1)
        goto err;

    if(fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
        goto err;

    return fd;

err:
    close(fd);
	warning("ny::wayland::setCloexecOrClose: failed");
    return -1;
}

int createTmpfileCloexec(char *tmpname)
{
    int fd;

    #ifdef HAVE_MKOSTEMP
		fd = mkostemp(tmpname, O_CLOEXEC);
		if(fd >= 0) unlink(tmpname);

    #else
		fd = mkstemp(tmpname);
		if(fd >= 0)
		{
			fd = setCloexecOrClose(fd);
			unlink(tmpname);
		}
    #endif

    return fd;
}

int osCreateAnonymousFile(off_t size)
{
    static const char template1[] = "/weston-shared-XXXXXX";
    const char *path;
    char *name;
    int fd;

    path = getenv("XDG_RUNTIME_DIR");
    if (!path)
    {
        errno = ENOENT;
        return -1;
    }

    name = (char*) malloc(strlen(path) + sizeof(template1));
    if(!name) return -1;

    strcpy(name, path);
    strcat(name, template1);

    fd = createTmpfileCloexec(name);
    free(name);

    if(fd < 0) return -1;

    if(ftruncate(fd, size) < 0)
    {
        close(fd);
        return -1;
    }

    return fd;
}

}

//shmBuffer
ShmBuffer::ShmBuffer(WaylandAppContext& ac, Vec2ui size, unsigned int stride) 
	: appContext_(&ac), size_(size), stride_(stride)
{
	format_ = WL_SHM_FORMAT_ARGB8888;
	if(!stride_) stride_ = size.x * 4;
    create();
}

ShmBuffer::~ShmBuffer()
{
    destroy();
}

ShmBuffer::ShmBuffer(ShmBuffer&& other)
{
	appContext_ = other.appContext_;
	shmSize_ = other.shmSize_;
	size_ = other.size_;
	stride_ = other.stride_;
	buffer_ = other.buffer_;
	pool_ = other.pool_;
	data_ = other.data_;
	format_ = other.format_;
	used_ = other.used_;

	other.appContext_ = {};
	other.shmSize_ = {};
	other.size_ = {};
	other.buffer_ = {};
	other.pool_ = {};
	other.data_ = {};
	other.format_ = {};
	other.used_ = {};

	if(buffer_) wl_buffer_set_user_data(buffer_, this);
}

ShmBuffer& ShmBuffer::operator=(ShmBuffer&& other)
{
	destroy();

	appContext_ = other.appContext_;
	shmSize_ = other.shmSize_;
	size_ = other.size_;
	stride_ = other.stride_;
	buffer_ = other.buffer_;
	pool_ = other.pool_;
	data_ = other.data_;
	format_ = other.format_;
	used_ = other.used_;

	other.appContext_ = {};
	other.shmSize_ = {};
	other.size_ = {};
	other.buffer_ = {};
	other.pool_ = {};
	other.data_ = {};
	other.format_ = {};
	other.used_ = {};

	if(buffer_) wl_buffer_set_user_data(buffer_, this);

	return *this;
}

void ShmBuffer::create()
{
	destroy();
	if(!size_.x || !size_.y) throw std::runtime_error("ny::wayland::ShmBuffer invalid size");
	if(!stride_) throw std::runtime_error("ny::wayland::ShmBuffer invalid stride");

    auto* shm = appContext_->wlShm();
    if(!shm) throw std::runtime_error("ny::wayland::ShmBuffer: wlAC has no wl_shm");

    auto vecSize = stride_ * size_.y;
    shmSize_ = std::max(vecSize, shmSize_);

    auto fd = osCreateAnonymousFile(shmSize_);
    if (fd < 0) throw std::runtime_error("ny::wayland::ShmBuffer: could not create shm file");

	//the fd is not needed here anymore AFTER the pool was created
	//our access to the file is represented by the pool
	auto fdGuard = nytl::makeScopeGuard([&]{ close(fd); });

    auto ptr = mmap(nullptr, shmSize_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(ptr == MAP_FAILED) throw std::runtime_error("ny::wayland::ShmBuffer: could not mmap file");

	data_ = reinterpret_cast<std::uint8_t*>(ptr);
    pool_ = wl_shm_create_pool(shm, fd, shmSize_);
    buffer_ = wl_shm_pool_create_buffer(pool_, 0, size_.x, size_.y, stride_, format_);

	static constexpr wl_buffer_listener listener
	{
		memberCallback<decltype(&ShmBuffer::released), &ShmBuffer::released, void(wl_buffer*)>
	};

    wl_buffer_add_listener(buffer_, &listener, this);
}

void ShmBuffer::destroy()
{
    if(buffer_) wl_buffer_destroy(buffer_);
    if(pool_) wl_shm_pool_destroy(pool_);
    if(data_) munmap(data_, shmSize_);
}

bool ShmBuffer::size(const Vec2ui& size, unsigned int stride)
{
    size_ = size;
	stride_ = stride;

	if(!stride_) stride_ = size.x * 4;
    unsigned int vecSize = stride_ * size_.y;

	if(!size_.x || !size_.y) throw std::runtime_error("ny::wayland::ShmBuffer invalid size");
	if(!stride_) throw std::runtime_error("ny::wayland::ShmBuffer invalid stride");

    if(vecSize > shmSize_)
    {
        create();
		return true;
    }
    else
    {
        wl_buffer_destroy(buffer_);
        buffer_ = wl_shm_pool_create_buffer(pool_, 0, size_.x, size_.y, stride_, format_);
		return false;
    }
}

//Callback
void callbackDone(void* data, wl_callback* callback, uint32_t callbackData)
{
    auto* call = static_cast<ServerCallback*>(data);
    call->done(*callback, callbackData);
}

const wl_callback_listener callbackListener =
{
    &callbackDone,
};

//
ServerCallback::ServerCallback(wl_callback& callback)
{
    wl_callback_add_listener(&callback, &callbackListener, this);
}

void ServerCallback::done(wl_callback& cb, unsigned int data)
{
    onCallback(cb, data);
}

//output
Output::Output(WaylandAppContext& ac, wl_output& outp, unsigned int id) 
	: appContext_(&ac), wlOutput_(&outp), globalID_(id)
{
	static constexpr wl_output_listener listener
	{
		memberCallback<decltype(&Output::geometry), &Output::geometry, 
			void(wl_output*, int32_t, int32_t, int32_t, int32_t, int32_t, const char*, 
				const char*, int32_t)>,

		memberCallback<decltype(&Output::mode), &Output::mode, 
			void(wl_output*, uint32_t, int32_t, int32_t, int32_t)>,

		memberCallback<decltype(&Output::done), &Output::done, void(wl_output*)>,
		memberCallback<decltype(&Output::scale), &Output::scale, void(wl_output*, int32_t)>,
	};

    wl_output_add_listener(&outp, &listener, this);
}

Output::~Output()
{
    if(wlOutput_) wl_output_release(wlOutput_);
}

Output::Output(Output&& other) noexcept :
	appContext_(other.appContext_), wlOutput_(other.wlOutput_), globalID_(other.globalID_),
	information_(other.information_)

{
	other.appContext_ = {};
	other.wlOutput_ = {};

	if(wlOutput_) wl_output_set_user_data(wlOutput_, this);
}

Output& Output::operator=(Output&& other) noexcept
{
    if(wlOutput_) wl_output_release(wlOutput_);

	appContext_ = other.appContext_;
	wlOutput_ = other.wlOutput_;
	globalID_ = other.globalID_;
	information_ = std::move(other.information_);

	other.appContext_ = {};
	other.wlOutput_ = {};

	if(wlOutput_) wl_output_set_user_data(wlOutput_, this);
	return *this;
}

void Output::geometry(int x, int y, int phwidth, int phheight, int subpixel, 
	const char* make, const char* model, int transform)
{
	information_.make = make;
	information_.model = model;
	information_.transform = transform;
	information_.position = {x, y};
	information_.physicalSize = Vec2ui(phwidth, phheight);
	information_.subpixel = subpixel;
}

void Output::mode(unsigned int flags, int width, int height, int refresh)
{
	unsigned int urefresh = refresh;
	information_.modes.push_back({Vec2ui(width, height), flags, urefresh});
}

void Output::done()
{
	information_.done = true;
}

void Output::scale(int scale)
{
	information_.scale = scale;
}

}//namespace wayland

WindowEdge waylandToEdge(unsigned int wlEdge)
{
	return static_cast<WindowEdge>(wlEdge);
}

unsigned int imageFormatToWayland(ImageDataFormat format)
{
    switch(format)
    {
		case ImageDataFormat::argb8888: return WL_SHM_FORMAT_ARGB8888;
		case ImageDataFormat::rgb888: return WL_SHM_FORMAT_RGB888;
		case ImageDataFormat::rgba8888: return WL_SHM_FORMAT_RGBA8888;
		case ImageDataFormat::bgr888: return WL_SHM_FORMAT_BGR888;
		case ImageDataFormat::bgra8888: return WL_SHM_FORMAT_BGRA8888;
		case ImageDataFormat::a8: return WL_SHM_FORMAT_C8;
        default: return -1;
    }
}
 
ImageDataFormat waylandToImageFormat(unsigned int wlFormat)
{
    switch(wlFormat)
    {
		case WL_SHM_FORMAT_ARGB8888: return ImageDataFormat::argb8888;
		case WL_SHM_FORMAT_RGB888: return ImageDataFormat::rgb888;
		case WL_SHM_FORMAT_BGRA8888: return ImageDataFormat::bgra8888;
		case WL_SHM_FORMAT_RGBA8888: return ImageDataFormat::rgba8888;
		case WL_SHM_FORMAT_XRGB8888: return ImageDataFormat::argb8888; //XXX: extra image format?
		case WL_SHM_FORMAT_C8: return ImageDataFormat::a8;
		default: return ImageDataFormat::none;
    }
}

}
