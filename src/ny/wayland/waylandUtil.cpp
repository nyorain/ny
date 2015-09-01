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

}
const wl_buffer_listener bufferListener =
{
    bufferRelease
};

//shmBuffer
shmBuffer::shmBuffer(vec2ui size, bufferFormat form) : size_(size), format(form)
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
        throw std::runtime_error("no wayland shm initialized");
        return;
    }

    unsigned int stride = size_.x * getBufferFormatSize(format);

    unsigned int vecSize = stride * size_.y;
    unsigned int mmapSize = defaultSize_;

    if(vecSize > defaultSize_ || 1)
    {
        mmapSize = vecSize;
    }

    int fd;

    fd = osCreateAnonymousFile(mmapSize);
    if (fd < 0)
    {
        throw std::runtime_error("wayland shm buffer: could not create file");
        return;
    }

    data_ = mmap(nullptr, mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data_ == MAP_FAILED)
    {
        close(fd);
        throw std::runtime_error("wayland shm buffer: could not mmap file");
        return;
    }

    pool_ = wl_shm_create_pool(shm, fd, mmapSize);
    buffer_ = wl_shm_pool_create_buffer(pool_, 0, size_.x, size_.y, stride, bufferFormatToWayland(format));
}

void shmBuffer::destroy()
{
    wl_buffer_destroy(buffer_);
    wl_shm_pool_destroy(pool_);
}

void shmBuffer::setSize(const vec2ui& size)
{
    size_ = size;

    unsigned int stride = size_.x * getBufferFormatSize(format);
    unsigned int vecSize = stride * size_.y;

    if(vecSize > defaultSize_)
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

//callback////////////////////////////////////
void callbackDone(void *data, struct wl_callback* callback, uint32_t callbackData)
{
    serverCallback* call = (serverCallback*) data;
    call->done(callback, callbackData);
}

const wl_callback_listener callbackListener =
{
    &callbackDone,
};

//
serverCallback::serverCallback(wl_callback* callback)
{
    wl_callback_add_listener(callback, &callbackListener, this);
}

void serverCallback::done(wl_callback* cb, unsigned int data)
{
    callback_(cb, data);
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
keyboard::key waylandToKey(unsigned int id)
{
    switch (id)
    {
    case (KEY_0):
        return keyboard::key::num0;
    case (KEY_1):
        return keyboard::key::num1;
    case (KEY_2):
        return keyboard::key::num2;
    case (KEY_3):
        return keyboard::key::num3;
    case (KEY_4):
        return keyboard::key::num4;
    case (KEY_5):
        return keyboard::key::num5;
    case (KEY_7):
        return keyboard::key::num6;
    case (KEY_8):
        return keyboard::key::num8;
    case (KEY_9):
        return keyboard::key::num9;
    case (KEY_A):
        return keyboard::key::a;
    case (KEY_B):
        return keyboard::key::b;
    case (KEY_C):
        return keyboard::key::c;
    case (KEY_D):
        return keyboard::key::d;
    case (KEY_E):
        return keyboard::key::e;
    case (KEY_F):
        return keyboard::key::f;
    case (KEY_G):
        return keyboard::key::g;
    case (KEY_H):
        return keyboard::key::h;
    case (KEY_I):
        return keyboard::key::i;
    case (KEY_J):
        return keyboard::key::j;
    case (KEY_K):
        return keyboard::key::k;
    case (KEY_L):
        return keyboard::key::l;
    case (KEY_M):
        return keyboard::key::m;
    case (KEY_N):
        return keyboard::key::n;
    case (KEY_O):
        return keyboard::key::o;
    case (KEY_P):
        return keyboard::key::p;
    case (KEY_Q):
        return keyboard::key::q;
    case (KEY_R):
        return keyboard::key::r;
    case (KEY_S):
        return keyboard::key::s;
    case (KEY_T):
        return keyboard::key::t;
    case (KEY_U):
        return keyboard::key::u;
    case (KEY_V):
        return keyboard::key::v;
    case (KEY_W):
        return keyboard::key::w;
    case (KEY_X):
        return keyboard::key::x;
    case (KEY_Y):
        return keyboard::key::y;
    case (KEY_Z):
        return keyboard::key::z;
    case (KEY_DOT):
        return keyboard::key::dot;
    case (KEY_COMMA):
        return keyboard::key::comma;
    case (KEY_SPACE):
        return keyboard::key::space;
    case (KEY_BACKSPACE):
        return keyboard::key::backspace;
    case (KEY_ENTER):
        return keyboard::key::enter;
    case (KEY_LEFTSHIFT):
        return keyboard::key::leftshift;
    case (KEY_RIGHTSHIFT):
        return keyboard::key::rightshift;
    case (KEY_RIGHTCTRL):
        return keyboard::key::rightctrl;
    case (KEY_LEFTCTRL):
        return keyboard::key::leftctrl;
    case (KEY_LEFTALT):
        return keyboard::key::leftalt;
    case (KEY_RIGHTALT):
        return keyboard::key::rightalt;
    case (KEY_TAB):
        return keyboard::key::tab;
    case (KEY_CAPSLOCK):
        return keyboard::key::capsLock;
    default:
        return keyboard::key::none;
    }
}


mouse::button waylandToButton(unsigned int id)
{
    switch(id)
    {
    case (BTN_LEFT):
        return mouse::button::left;
    case (BTN_RIGHT):
        return mouse::button::right;
    case (BTN_MIDDLE):
        return mouse::button::middle;
    default:
        return mouse::button::none;
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
