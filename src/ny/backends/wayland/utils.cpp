#include <ny/backends/wayland/utils.hpp>

#include <ny/backends/wayland/appContext.hpp>
#include <ny/backends/wayland/windowContext.hpp>
#include <ny/backends/wayland/cairo.hpp>

#ifdef NY_WithGL
#include <ny/backends/wayland/gl.hpp>
#endif // NY_WithGL

#include <ny/app/app.hpp>
#include <ny/app/event.hpp>
#include <ny/app/cursor.hpp>

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

//shellSurface///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void shellSurfaceHandlePing(void* data,  wl_shell_surface *shellSurface, uint32_t serial)
{
    wl_shell_surface_pong(shellSurface, serial);
}

void shellSurfaceHandleConfigure(void* data,  wl_shell_surface *shellSurface, uint32_t edges, int32_t width, int32_t height)
{
    waylandAppContext* a = dynamic_cast<waylandAppContext*>(getMainApp()->getAppContext());

    if(!a)
        return;

    a->eventWindowResized(shellSurface, edges, width, height);
}

void shellSurfaceHandlePopupDone(void* data, wl_shell_surface *shellSurface)
{
}

//frame///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void surfaceHandleFrame(void* data,  wl_callback *callback, uint32_t time)
{
    waylandWC* wc =  (waylandWindowContext*)data;

    waylandFrameEvent ev;
    ev.handler = &wc->getWindow();
    ev.backend = Wayland;

    getMainApp()->sendEvent(ev, *ev.handler);
}

//global Registry/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void globalRegistryHandleAdd(void* data,  wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
{
    waylandAppContext* a = (waylandAppContext*) data;
    a->registryHandler(registry, id, interface, version);
}

void globalRegistryHandleRemove(void* data,  wl_registry* registry, uint32_t id)
{
    waylandAppContext* a = (waylandAppContext*) data;
    a->registryRemover(registry, id);
}

//shm////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void shmHandleFormat(void* data,  wl_shm* shm, uint32_t format)
{
    waylandAppContext* a = (waylandAppContext*) data;

    a->shmFormat(shm, format);
}

//seat///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void seatHandleCapabilities(void* data,  wl_seat* seat, unsigned int caps)
{
    waylandAppContext* a = (waylandAppContext*) data;
    a->seatCapabilities(seat, caps);
}

//pointer////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void pointerHandleEnter(void* data, wl_pointer *pointer, uint32_t serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventMouseEnterSurface(pointer, serial, surface, sx, sy);
}

void pointerHandleLeave(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventMouseLeaveSurface(pointer, serial, surface);
}

void pointerHandleMotion(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventMouseMove(pointer, time, sx, sy);
}

void pointerHandleButton(void* data,  wl_pointer* pointer,  uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventMouseButton(pointer,serial, time, button, state);
}

void pointerHandleAxis(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventMouseAxis(pointer, time, axis, value);
}


//keyboard////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void keyboardHandleKeymap(void* data, wl_keyboard* keyboard, uint32_t format, int fd, uint32_t size)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventKeyboardKeymap(keyboard, format, fd, size);
}

void keyboardHandleEnter(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventKeyboardEnterSurface(keyboard, serial, surface, keys);
}

void keyboardHandleLeave(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventKeyboardLeaveSurface(keyboard, serial, surface);
}

void keyboardHandleKey(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventKeyboardKey(keyboard, serial, time, key, state);
}

void keyboardHandleModifiers(void* data, wl_keyboard* keyboard,uint32_t serial, uint32_t modsDepressed,uint32_t modsLatched, uint32_t modsLocked, uint32_t group)
{
    waylandAppContext* app = (waylandAppContext*) data;
    app->eventKeyboardModifiers(keyboard, serial, modsDepressed, modsLatched, modsLocked, group);
}

//displaySnyc//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void displayHandleSync(void* data, wl_callback* callback, uint32_t time)
{
}


//buffer///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

shmBuffer::shmBuffer(wl_shm* shm, unsigned int size, bufferFormat format)
{
    unsigned int stride = size.x * 4; // 4 bytes per pixel
    int fd;

    fd = osCreateAnonymousFile(size);
    if (fd < 0)
    {
        std::cout << "creating a buffer file failed" << std::endl;
        return 0;
    }

    pixels_ = (unsigned char*) mmap(nullptr, 100000000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pixels_ == MAP_FAILED)
    {
        std::cout << "mmap failed" << std::endl;
        close(fd);
        return 0;
    }

    wlShmPool_ = wl_shm_create_pool(shm, fd, 100000000);
    wlBuffer_ = wl_shm_pool_create_buffer(wlShmPool_, 0, size.x, size.y, stride, WL_SHM_FORMAT_ARGB8888);

    return 1;
}

shmBuffer::~shmBuffer()
{
    wl_buffer_destroy(buffer);
}

}//end namespace wayland

//utils////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
    case cursorType::LeftPtr:
        return "left_ptr";
    case cursorType::SizeBottom:
        return "bottom_side";
    case cursorType::SizeBottomLeft:
        return "bottom_left_corner";
    case cursorType::SizeBottomRight:
        return "bottom_right_corner";
    case cursorType::SizeTop:
        return "top_side";
    case cursorType::SizeTopLeft:
        return "top_left_corner";
    case cursorType::SizeTopRight:
        return "top_right_corner";
    case cursorType::SizeLeft:
        return "left_side";
    case cursorType::SizeRight:
        return "right_side";
    case cursorType::Grab:
        return "grabbing";
    default:
        return "";
    }
}

cursorType waylandToCursor(std::string id)
{
    //if(id == "fleur") return cursorType::Move;
    if(id == "left_ptr") return cursorType::LeftPtr;
    if(id == "bottom_side") return cursorType::SizeBottom;
    if(id == "left_side") return cursorType::SizeLeft;
    if(id == "right_side") return cursorType::SizeRight;
    if(id == "top_side") return cursorType::SizeTop;
    if(id == "top_side") return cursorType::SizeTop;
    if(id == "top_left_corner") return cursorType::SizeTopLeft;
    if(id == "top_right_corner") return cursorType::SizeTopRight;
    if(id == "bottom_right_corner") return cursorType::SizeBottomRight;
    if(id == "bottom_left_corner") return cursorType::SizeBottomLeft;
    if(id == "grabbing") return cursorType::Grab;
    return cursorType::Unknown;
}

unsigned int bufferFormatToWayland(bufferFormat format)
{
    switch(format)
    {
        case bufferFormat::argb8888: return WL_SHM_FORMAT_ARGB8888;
        default: return 0;
    }
}

bufferFormat waylandToBufferFormat(unsigned int wlFormat)
{
    switch(wlFormat)
    {
        case WL_SHM_FORMAT_ABGR8888: return bufferFormat::argb8888;
        default: return bufferFormat::Unknown;
    }
}


//conversions from waylandInclude
waylandAppContext* asWayland(appContext* c){ return dynamic_cast<waylandAppContext*>(c); };
waylandWindowContext* asWayland(windowContext* c){ return dynamic_cast<waylandWindowContext*>(c); };
waylandToplevelWindowContext* asWayland(toplevelWindowContext* c){ return dynamic_cast<waylandToplevelWindowContext*>(c); };
waylandChildWindowContext* asWayland(childWindowContext* c){ return dynamic_cast<waylandChildWindowContext*>(c); };
waylandChildWindowContext* asWaylandChild(windowContext* c){ return dynamic_cast<waylandChildWindowContext*>(c); };
waylandToplevelWindowContext* asWaylandToplevel(windowContext* c){ return dynamic_cast<waylandToplevelWindowContext*>(c); };

waylandCairoToplevelWindowContext* asWaylandCairo(toplevelWindowContext* c){ return dynamic_cast<waylandCairoToplevelWindowContext*>(c); };
waylandCairoChildWindowContext* asWaylandCairo(childWindowContext* c){ return dynamic_cast<waylandCairoChildWindowContext*>(c); };
waylandCairoContext* asWaylandCairo(windowContext* c){ return dynamic_cast<waylandCairoContext*>(c); };

#ifdef NY_WithGL
waylandGLToplevelWindowContext* asWaylandGL(toplevelWindowContext* c){ return dynamic_cast<waylandGLToplevelWindowContext*>(c); };
waylandGLChildWindowContext* asWaylandGL(childWindowContext* c){ return dynamic_cast<waylandGLChildWindowContext*>(c); };
waylandGLContext* asWaylandGL(windowContext* c){ return dynamic_cast<waylandGLContext*>(c); };
#endif // NY_WithGL


}
