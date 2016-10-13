#include <ny/backend/wayland/util.hpp>

#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/cairo.hpp>

#ifdef NY_WithGL
#include <ny/backend/wayland/egl.hpp>
#endif // NY_WithGL

#include <ny/base/event.hpp>
#include <ny/base/log.hpp>
#include <ny/base/cursor.hpp>
#include <ny/base/imageData.hpp>
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

//buffer
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

//buffer interface
void bufferRelease(void* data, wl_buffer*)
{
    auto* b = static_cast<ShmBuffer*>(data);
    b->wasReleased();
}
const wl_buffer_listener bufferListener =
{
    bufferRelease
};

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
}

ShmBuffer& ShmBuffer::operator=(ShmBuffer&& other)
{
	destroy();

	appContext_ = other.appContext_;
	shmSize_ = other.shmSize_;
	size_ = other.size_;
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

	return *this;
}

void ShmBuffer::create()
{
	destroy();

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
    wl_buffer_add_listener(buffer_, &bufferListener, this);
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
void outputGeometry(void* data, wl_output*, int x, int y, int phwidth, int phheight, int subpixel, 
	const char* make, const char* model, int transform)
{
	auto* output = static_cast<Output*>(data);
	output->make = make;
	output->model = model;
	output->transform = transform;
	output->position = {x, y};
	output->physicalSize = Vec2ui(phwidth, phheight);
	output->subpixel = subpixel;
}
void outputMode(void* data, wl_output*, unsigned int flags, int width, int height, int refresh)
{
	auto* output = static_cast<Output*>(data);
	unsigned int urefresh = refresh;
	output->modes.push_back({Vec2ui(width, height), flags, urefresh});
}
void outputDone(void* data, wl_output*)
{
	auto* output = static_cast<Output*>(data);
	output->appContext->outputDone(*output);
}
void outputScale(void* data, wl_output*, int factor)
{
	auto* output = static_cast<Output*>(data);
	output->scale = factor;
}
const wl_output_listener outputListener =
{
    outputGeometry,
    outputMode,
    outputDone,
    outputScale
};

//output
Output::Output(WaylandAppContext& ac, wl_output& outp, unsigned int id) 
	: appContext(&ac), wlOutput(&outp), globalID(id)
{
    wl_output_add_listener(&outp, &outputListener, this);
}

Output::~Output()
{
    if(wlOutput) wl_output_destroy(wlOutput);
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
