#include <ny/wayland/waylandUtil.hpp>

#include <ny/wayland/waylandAppContext.hpp>
#include <ny/wayland/waylandWindowContext.hpp>
#include <ny/wayland/waylandCairo.hpp>

#ifdef NY_WithGL
#include <ny/wayland/waylandEgl.hpp>
#endif // NY_WithGL

#include <ny/app.hpp>
#include <ny/event.hpp>
#include <ny/cursor.hpp>
#include <ny/error.hpp>

#include <wayland-cursor.h>

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

//buffer///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
void bufferRelease(void* data, wl_buffer* wl_buffer)
{
    shmBuffer* b = (shmBuffer*) data;
    b->wasReleased();
}
const wl_buffer_listener bufferListener =
{
    bufferRelease
};

//shmBuffer
shmBuffer::shmBuffer(Vec2ui size, bufferFormat form) : size_(size), format(form)
{
    create();
}

shmBuffer::~shmBuffer()
{
    destroy();
}

void shmBuffer::create()
{
    waylandAppContext* ac;
    if(!(ac = getWaylandAppContext()))
    {
        throw std::runtime_error("need wayland appContext to create wayland shm buffer");
        return;
    }

    if(!ac->bufferFormatSupported(format))
    {
        throw std::runtime_error("wayland shm buffer: format not supported");
        return;
    }

    wl_shm* shm = ac->getWlShm();
    if(!shm)
    {
        throw std::runtime_error("wayland shm buffer: no wayland shm initialized");
        return;
    }

    unsigned int stride = size_.x * getBufferFormatSize(format);

    unsigned int VecSize = stride * size_.y;
    shmSize_ = std::max(VecSize, shmSize_);

    int fd;

    fd = osCreateAnonymousFile(shmSize_);
    if (fd < 0)
    {
        throw std::runtime_error("wayland shm buffer: could not create file");
        return;
    }

    data_ = mmap(nullptr, shmSize_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data_ == MAP_FAILED)
    {
        close(fd);
        throw std::runtime_error("wayland shm buffer: could not mmap file");
        return;
    }

    pool_ = wl_shm_create_pool(shm, fd, shmSize_);
    buffer_ = wl_shm_pool_create_buffer(pool_, 0, size_.x, size_.y, stride, bufferFormatToWayland(format));
    wl_buffer_add_listener(buffer_, &bufferListener, this);
}

void shmBuffer::destroy()
{
    if(buffer_) wl_buffer_destroy(buffer_);
    if(pool_) wl_shm_pool_destroy(pool_);
    if(data_) munmap(data_, shmSize_);
}

void shmBuffer::setSize(const Vec2ui& size)
{
    size_ = size;

    unsigned int stride = size_.x * getBufferFormatSize(format);
    unsigned int VecSize = stride * size_.y;

    if(VecSize > shmSize_)
    {
        destroy();
        create();
    }
    else
    {
        wl_buffer_destroy(buffer_);
        buffer_ = wl_shm_pool_create_buffer(pool_, 0, size_.x, size_.y, stride, bufferFormatToWayland(format));
    }
}

//Callback////////////////////////////////////
void CallbackDone(void *data, struct wl_Callback* Callback, uint32_t CallbackData)
{
    serverCallback* call = (serverCallback*) data;
    call->done(Callback, CallbackData);
}

const wl_Callback_listener CallbackListener =
{
    &CallbackDone,
};

//
serverCallback::serverCallback(wl_Callback* Callback)
{
    wl_Callback_add_listener(Callback, &CallbackListener, this);
}

void serverCallback::done(wl_Callback* cb, unsigned int data)
{
    Callback_(cb, data);
}


//output
void outputGeometry(void* data, wl_output* wl_output, int x, int y, int physical_width, int physical_height, int subpixel, const char* make, const char* model, int transform)
{

}
void outputMode(void* data, wl_output* wl_output, unsigned int flags, int width, int height, int refresh)
{

}
void outputDone(void* data, wl_output* wl_output)
{

}
void outputScale(void* data, wl_output* wl_output, int factor)
{

}
const wl_output_listener outputListener =
{
    outputGeometry,
    outputMode,
    outputDone,
    outputScale
};

//output
output::output(wl_output* outp) : wlOutput_(outp)
{
    wl_output_add_listener(outp, &outputListener, this);
}

output::~output()
{
    wl_output_destroy(wlOutput_);
}

}//end namespace wayland

//util////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Key linuxToKey(unsigned int id)
{
    switch (id)
    {
    case (KEY_0):
        return Key::num0;
    case (KEY_1):
        return Key::num1;
    case (KEY_2):
        return Key::num2;
    case (KEY_3):
        return Key::num3;
    case (KEY_4):
        return Key::num4;
    case (KEY_5):
        return Key::num5;
    case (KEY_7):
        return Key::num6;
    case (KEY_8):
        return Key::num8;
    case (KEY_9):
        return Key::num9;
    case (KEY_A):
        return Key::a;
    case (KEY_B):
        return Key::b;
    case (KEY_C):
        return Key::c;
    case (KEY_D):
        return Key::d;
    case (KEY_E):
        return Key::e;
    case (KEY_F):
        return Key::f;
    case (KEY_G):
        return Key::g;
    case (KEY_H):
        return Key::h;
    case (KEY_I):
        return Key::i;
    case (KEY_J):
        return Key::j;
    case (KEY_K):
        return Key::k;
    case (KEY_L):
        return Key::l;
    case (KEY_M):
        return Key::m;
    case (KEY_N):
        return Key::n;
    case (KEY_O):
        return Key::o;
    case (KEY_P):
        return Key::p;
    case (KEY_Q):
        return Key::q;
    case (KEY_R):
        return Key::r;
    case (KEY_S):
        return Key::s;
    case (KEY_T):
        return Key::t;
    case (KEY_U):
        return Key::u;
    case (KEY_V):
        return Key::v;
    case (KEY_W):
        return Key::w;
    case (KEY_X):
        return Key::x;
    case (KEY_Y):
        return Key::y;
    case (KEY_Z):
        return Key::z;
    case (KEY_DOT):
        return Key::dot;
    case (KEY_COMMA):
        return Key::comma;
    case (KEY_SPACE):
        return Key::space;
    case (KEY_BACKSPACE):
        return Key::backspace;
    case (KEY_ENTER):
        return Key::enter;
    case (KEY_LEFTSHIFT):
        return Key::leftshift;
    case (KEY_RIGHTSHIFT):
        return Key::rightshift;
    case (KEY_RIGHTCTRL):
        return Key::rightctrl;
    case (KEY_LEFTCTRL):
        return Key::leftctrl;
    case (KEY_LEFTALT):
        return Key::leftalt;
    case (KEY_RIGHTALT):
        return Key::rightalt;
    case (KEY_TAB):
        return Key::tab;
    case (KEY_CAPSLOCK):
        return Key::capsLock;
    default:
        return Key::none;
    }
}


Button linuxToButton(unsigned int id)
{
    switch(id)
    {
    case BTN_LEFT:
        return mouse::button::left;
    case BTN_RIGHT:
        return mouse::button::right;
    case BTN_MIDDLE:
        return mouse::button::middle;
	case BTN_FORWARD:
		return Button::custom1;
	case BTN_BACK:
		return Button::custom2;
	case BTN_SIDE:
		return Button::custom3;
	case BTN_EXTRA:
		return Button::custom4;
	case BTN_TASK:
		return Button::custom5;
    default:
        return Button::none;
    }
}


std::string cursorToWayland(const cursorType c)
{
    switch(c)
    {
    case cursorType::leftPtr:
        return "left_ptr";
    case cursorType::sizeBottom:
        return "bottom_side";
    case cursorType::sizeBottomLeft:
        return "bottom_left_corner";
    case cursorType::sizeBottomRight:
        return "bottom_right_corner";
    case cursorType::sizeTop:
        return "top_side";
    case cursorType::sizeTopLeft:
        return "top_left_corner";
    case cursorType::sizeTopRight:
        return "top_right_corner";
    case cursorType::sizeLeft:
        return "left_side";
    case cursorType::sizeRight:
        return "right_side";
    case cursorType::grab:
        return "grabbing";
    default:
        return "";
    }
}

cursorType waylandToCursor(std::string id)
{
    //if(id == "fleur") return cursorType::Move;
    if(id == "left_ptr") return cursorType::leftPtr;
    if(id == "bottom_side") return cursorType::sizeBottom;
    if(id == "left_side") return cursorType::sizeLeft;
    if(id == "right_side") return cursorType::sizeRight;
    if(id == "top_side") return cursorType::sizeTop;
    if(id == "top_side") return cursorType::sizeTop;
    if(id == "top_left_corner") return cursorType::sizeTopLeft;
    if(id == "top_right_corner") return cursorType::sizeTopRight;
    if(id == "bottom_right_corner") return cursorType::sizeBottomRight;
    if(id == "bottom_left_corner") return cursorType::sizeBottomLeft;
    if(id == "grabbing") return cursorType::grab;
    return cursorType::unknown;
}

int bufferFormatToWayland(bufferFormat format)
{
    switch(format)
    {
        case bufferFormat::argb8888: return WL_SHM_FORMAT_ARGB8888;
        case bufferFormat::xrgb8888: return WL_SHM_FORMAT_XRGB8888;
        case bufferFormat::rgb888: return WL_SHM_FORMAT_RGB888;
        default: return -1;
    }
}

bufferFormat waylandToBufferFormat(unsigned int wlFormat)
{
    switch(wlFormat)
    {
        case WL_SHM_FORMAT_ABGR8888: return bufferFormat::argb8888;
        case WL_SHM_FORMAT_XRGB8888: return bufferFormat::xrgb8888;
        case WL_SHM_FORMAT_RGB888: return bufferFormat::rgb888;
        default: return bufferFormat::unknown;
    }
}


//conversions from waylandInclude
waylandAppContext* asWayland(appContext* c){ return dynamic_cast<waylandAppContext*>(c); };
waylandWindowContext* asWayland(windowContext* c){ return dynamic_cast<waylandWindowContext*>(c); };

waylandAppContext* getWaylandAppContext()
{
    waylandAppContext* ret = nullptr;

    if(nyMainApp())
    {
        ret = dynamic_cast<waylandAppContext*>(nyMainApp()->getAppContext());
    }

    return ret;
}

waylandAppContext* getWaylandAC()
{
    return getWaylandAppContext();
}


}
