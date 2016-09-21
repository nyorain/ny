#include <ny/backend/wayland/util.hpp>

#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/cairo.hpp>

#ifdef NY_WithGL
#include <ny/backend/wayland/egl.hpp>
#endif // NY_WithGL

#include <ny/base/event.hpp>
#include <ny/base/cursor.hpp>
#include <ny/base/imageData.hpp>

#include <wayland-cursor.h>
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

    if (fd == -1)
        return -1;

    flags = fcntl(fd, F_GETFD);
    if (flags == -1)
        goto err;

    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
        goto err;

    return fd;

err:
    close(fd);
    std::cout << "error" << std::endl;
    return -1;
}

int createTmpfileCloexec(char *tmpname)
{
    int fd;

    #ifdef HAVE_MKOSTEMP
    fd = mkostemp(tmpname, O_CLOEXEC);
    if (fd >= 0)
        unlink(tmpname);
    #else
    fd = mkstemp(tmpname);
    if (fd >= 0)
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
    if (!name)
        return -1;
    strcpy(name, path);
    strcat(name, template1);

    fd = createTmpfileCloexec(name);

    free(name);

    if (fd < 0)
        return -1;

    if (ftruncate(fd, size) < 0)
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
ShmBuffer::ShmBuffer(WaylandAppContext& ac, Vec2ui size) : appContext_(&ac), size_(size)
{
	format_ = WL_SHM_FORMAT_XRGB8888;
	if(appContext_->shmFormatSupported(WL_SHM_FORMAT_ARGB8888))
		format_ = WL_SHM_FORMAT_ARGB8888;
	else if(appContext_->shmFormatSupported(WL_SHM_FORMAT_BGRA8888))
		format_ = WL_SHM_FORMAT_BGRA8888;
	else if(appContext_->shmFormatSupported(WL_SHM_FORMAT_ABGR8888))
		format_ = WL_SHM_FORMAT_ABGR8888;
	else if(appContext_->shmFormatSupported(WL_SHM_FORMAT_RGBA8888))
		format_ = WL_SHM_FORMAT_RGBA8888;

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
    auto* shm = appContext_->wlShm();
    if(!shm)
    {
        throw std::runtime_error("wayland shm buffer: no wayland shm initialized");
        return;
    }

    auto stride = size_.x * 4;
    auto vecSize = stride * size_.y;
    shmSize_ = std::max(vecSize, shmSize_);

    auto fd = osCreateAnonymousFile(shmSize_);
    if (fd < 0)
    {
        throw std::runtime_error("wayland shm buffer: could not create file");
        return;
    }

    auto ptr = mmap(nullptr, shmSize_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data_ == MAP_FAILED)
    {
        close(fd);
        throw std::runtime_error("wayland shm buffer: could not mmap file");
        return;
    }

	data_ = reinterpret_cast<std::uint8_t*>(ptr);
    pool_ = wl_shm_create_pool(shm, fd, shmSize_);
    buffer_ = wl_shm_pool_create_buffer(pool_, 0, size_.x, size_.y, stride, format_);
    wl_buffer_add_listener(buffer_, &bufferListener, this);
}

void ShmBuffer::destroy()
{
    if(buffer_) wl_buffer_destroy(buffer_);
    if(pool_) wl_shm_pool_destroy(pool_);
    if(data_) munmap(data_, shmSize_);
}

void ShmBuffer::size(const Vec2ui& size)
{
    size_ = size;

    unsigned int stride = size_.x * 4;
    unsigned int VecSize = stride * size_.y;

    if(VecSize > shmSize_)
    {
        destroy();
        create();
    }
    else
    {
        wl_buffer_destroy(buffer_);
        buffer_ = wl_shm_pool_create_buffer(pool_, 0, size_.x, size_.y, stride, format_);
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
    wl_output_destroy(wlOutput);
}

}//namespace wayland

//util
Key linuxToKey(unsigned int id)
{
    switch (id)
    {
		case (KEY_0): return Key::n0;
		case (KEY_1): return Key::n1;
		case (KEY_2): return Key::n2;
		case (KEY_3): return Key::n3;
		case (KEY_4): return Key::n4;
		case (KEY_5): return Key::n5;
		case (KEY_7): return Key::n6;
		case (KEY_8): return Key::n8;
		case (KEY_9): return Key::n9;
		case (KEY_A): return Key::a;
		case (KEY_B): return Key::b;
		case (KEY_C): return Key::c;
		case (KEY_D): return Key::d;
		case (KEY_E): return Key::e;
		case (KEY_F): return Key::f;
		case (KEY_G): return Key::g;
		case (KEY_H): return Key::h;
		case (KEY_I): return Key::i;
		case (KEY_J): return Key::j;
		case (KEY_K): return Key::k;
		case (KEY_L): return Key::l;
		case (KEY_M): return Key::m;
		case (KEY_N): return Key::n;
		case (KEY_O): return Key::o;
		case (KEY_P): return Key::p;
		case (KEY_Q): return Key::q;
		case (KEY_R): return Key::r;
		case (KEY_S): return Key::s;
		case (KEY_T): return Key::t;
		case (KEY_U): return Key::u;
		case (KEY_V): return Key::v;
		case (KEY_W): return Key::w;
		case (KEY_X): return Key::x;
		case (KEY_Y): return Key::y;
		case (KEY_Z): return Key::z;
		case (KEY_DOT): return Key::dot;
		case (KEY_COMMA): return Key::comma;
		case (KEY_SPACE): return Key::space;
		case (KEY_BACKSPACE): return Key::backspace;
		case (KEY_ENTER): return Key::enter;
		case (KEY_LEFTSHIFT): return Key::leftshift;
		case (KEY_RIGHTSHIFT): return Key::rightshift;
		case (KEY_RIGHTCTRL): return Key::rightctrl;
		case (KEY_LEFTCTRL): return Key::leftctrl;
		case (KEY_LEFTALT): return Key::leftalt;
		case (KEY_RIGHTALT): return Key::rightalt;
		case (KEY_TAB): return Key::tab;
		case (KEY_CAPSLOCK): return Key::capsLock;
		default: return Key::none;
    }
}


MouseButton linuxToButton(unsigned int id)
{
    switch(id)
    {
		case BTN_LEFT: return MouseButton::left;
		case BTN_RIGHT: return MouseButton::right;
		case BTN_MIDDLE: return MouseButton::middle;
		case BTN_FORWARD: return MouseButton::custom1;
		case BTN_BACK: return MouseButton::custom2;
		case BTN_SIDE: return MouseButton::custom3;
		case BTN_EXTRA: return MouseButton::custom4;
		case BTN_TASK: return MouseButton::custom5;
		default: return MouseButton::none;
    }
}


std::string cursorToWayland(const CursorType c)
{
    switch(c)
    {
		case CursorType::leftPtr: return "left_ptr";
		case CursorType::sizeBottom: return "bottom_side";
		case CursorType::sizeBottomLeft: return "bottom_left_corner";
		case CursorType::sizeBottomRight: return "bottom_right_corner";
		case CursorType::sizeTop: return "top_side";
		case CursorType::sizeTopLeft: return "top_left_corner";
		case CursorType::sizeTopRight: return "top_right_corner";
		case CursorType::sizeLeft: return "left_side";
		case CursorType::sizeRight: return "right_side";
		case CursorType::grab: return "grabbing";
		default: return "";
    }
}

CursorType waylandToCursor(std::string id)
{
    //if(id == "fleur") return cursorType::Move;
    if(id == "left_ptr") return CursorType::leftPtr;
    if(id == "bottom_side") return CursorType::sizeBottom;
    if(id == "left_side") return CursorType::sizeLeft;
    if(id == "right_side") return CursorType::sizeRight;
    if(id == "top_side") return CursorType::sizeTop;
    if(id == "top_side") return CursorType::sizeTop;
    if(id == "top_left_corner") return CursorType::sizeTopLeft;
    if(id == "top_right_corner") return CursorType::sizeTopRight;
    if(id == "bottom_right_corner") return CursorType::sizeBottomRight;
    if(id == "bottom_left_corner") return CursorType::sizeBottomLeft;
    if(id == "grabbing") return CursorType::grab;
    return CursorType::unknown;
}

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
		case WL_SHM_FORMAT_ABGR8888: return ImageDataFormat::argb8888;
		case WL_SHM_FORMAT_RGB888: return ImageDataFormat::rgb888;
		case WL_SHM_FORMAT_BGRA8888: return ImageDataFormat::bgra8888;
		case WL_SHM_FORMAT_RGBA8888: return ImageDataFormat::rgba8888;
		case WL_SHM_FORMAT_XRGB8888: return ImageDataFormat::argb8888;
		case WL_SHM_FORMAT_C8: return ImageDataFormat::a8;
		default: return ImageDataFormat::none;
    }
}

}
