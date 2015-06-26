#include "backends/wayland/appContext.hpp"

#include "backends/wayland/utils.hpp"
#include "backends/wayland/windowContext.hpp"

#include "app/error.hpp"
#include "app/app.hpp"

#include "utils/misc.hpp"

#include <wayland-egl.h>

namespace ny
{

using namespace wayland;

//waylandAppContext//////////////////////////////////////////////////////////////////////////
waylandAppContext::waylandAppContext() : appContext()
{
    wlDisplay_ = nullptr;
    wlCompositor_ = nullptr;
    wlShell_ = nullptr;
    wlSeat_ = nullptr;
    wlSubcompositor_ = nullptr;
    wlShm_ = nullptr;
    wlPointer_ = nullptr;
    wlKeyboard_ = nullptr;

    init();
}

waylandAppContext::~waylandAppContext()
{
    if(wlDisplay_)
    {
        wl_display_disconnect(wlDisplay_);
    }
}

void waylandAppContext::init()
{
    //init display
    wlDisplay_ = wl_display_connect(nullptr);

    if(!wlDisplay_)
    {
        throw error(error::Critical, "could not connect to wayland display");
        return;
    }

    //init registry
    wl_registry *registry = wl_display_get_registry(wlDisplay_);
    wl_registry_add_listener(registry, &globalRegistryListener, this);

    wl_display_dispatch(wlDisplay_);
    wl_display_roundtrip(wlDisplay_);

    //compositor added by registry callback listener
    if(!wlCompositor_)
    {
        throw error(error::Critical, "could not find wayland compositor");
        return;
    }

    wl_callback* syncer = wl_display_sync(wlDisplay_);
    wl_callback_add_listener(syncer, &displaySyncListener, this);

    wlCursorSurface_ = wl_compositor_create_surface(wlCompositor_);

#ifdef WithGL
    eglContext_ = new waylandEGLAppContext(this);
    if(!eglContext_->init())
    {
        throw error(error::Critical, "could not find initialize wayland eglContext");
        return;
    }
#endif // WithGL
}

bool waylandAppContext::mainLoopCall()
{
    if(wl_display_dispatch(wlDisplay_) == -1)
        return 0;

    return 1;
}

void waylandAppContext::registryHandler(wl_registry* registry, unsigned int id, std::string interface, unsigned int version)
{
    if (interface == "wl_compositor")
    {
        wlCompositor_ = (wl_compositor*) wl_registry_bind(registry, id, &wl_compositor_interface, version);
    }

    else if (interface == "wl_shell")
    {
        wlShell_ = (wl_shell*) wl_registry_bind(registry, id, &wl_shell_interface, version);
    }

    else if (interface == "wl_shm")
    {
        wlShm_ = (wl_shm*) wl_registry_bind(registry, id, &wl_shm_interface, version);
        wl_shm_add_listener(wlShm_, &shmListener, this);

        wlCursorTheme_ = wl_cursor_theme_load(nullptr, 32, wlShm_);
    }

    else if(interface == "wl_subcompositor")
    {
        wlSubcompositor_ = (wl_subcompositor*) wl_registry_bind(registry, id, &wl_subcompositor_interface, version);
    }

    else if(interface == "wl_seat")
    {
        wlSeat_ = (wl_seat*) wl_registry_bind(registry, id, &wl_seat_interface, version);
        wl_seat_add_listener(wlSeat_, &seatListener, this);
    }

    else if(interface == "wl_data_device_manager")
    {
        wlDataManager_ = (wl_data_device_manager*) wl_registry_bind(registry, id, &wl_data_device_manager_interface, version);
    }

}

void waylandAppContext::registryRemover(wl_registry* registry, unsigned int id)
{
}

void waylandAppContext::seatCapabilities(wl_seat* seat, unsigned int caps)
{
    if ((caps & WL_SEAT_CAPABILITY_POINTER))
    {
        wlPointer_ = wl_seat_get_pointer(wlSeat_);
        wl_pointer_add_listener(wlPointer_, &pointerListener, this);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && wlPointer_)
    {
        wl_pointer_destroy(wlPointer_);
        wlPointer_ = nullptr;
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD))
    {
        wlKeyboard_ = wl_seat_get_keyboard(wlSeat_);
        wl_keyboard_add_listener(wlKeyboard_, &keyboardListener, this);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && wlKeyboard_)
    {
        wl_keyboard_destroy(wlKeyboard_);
        wlKeyboard_ = nullptr;
    }

}

void waylandAppContext::eventMouseMove(wl_pointer *pointer, unsigned int time, int sx, int sy)
{
    mouseMoveEvent e;

    e.position = vec2i(wl_fixed_to_int(sx), wl_fixed_to_int(sy));
    e.delta = e.position - mouse::getPosition();

    e.backend = Wayland;

    getMainApp()->mouseMove(e);
}

void waylandAppContext::eventMouseEnterSurface(wl_pointer *pointer, unsigned int serial, wl_surface *surface, int sx, int sy)
{
    if(!surface) return;

    mouseCrossEvent e;
    e.state = crossType::entered;

    e.backend = Wayland;

    void* data = wl_surface_get_user_data(surface);

    if(!data) return;

    waylandWC* con = static_cast<waylandWC*>(data);
    e.handler = &con->getWindow();

    e.data = new waylandEventData(serial);
    e.position = vec2i(wl_fixed_to_int(sx), wl_fixed_to_int(sy));

    getMainApp()->mouseCross(e);
}

void waylandAppContext::eventMouseLeaveSurface(wl_pointer *pointer, unsigned int serial, wl_surface *surface)
{
    if(!surface) return;

    mouseCrossEvent e;
    e.state = crossType::left;

    e.backend = Wayland;

    void* data = wl_surface_get_user_data(surface);

    if(!data) return;

    waylandWC* con = static_cast<waylandWC*>(data);
    e.handler = &con->getWindow();

    e.data = new waylandEventData(serial);

    getMainApp()->mouseCross(e);
}

void waylandAppContext::eventMouseButton(wl_pointer *wl_pointer, unsigned int serial, unsigned int time, unsigned int button, unsigned int state)
{
    mouseButtonEvent e;

    e.button = waylandToButton(button);
    if(state == 1) e.state = pressState::pressed;
    else if(state == 0) e.state = pressState::released;

    e.backend = Wayland;
    e.data = new waylandEventData(serial);
    e.position = mouse::getPosition();

    getMainApp()->mouseButton(e);
}

void waylandAppContext::eventMouseAxis(wl_pointer *pointer, unsigned int time, unsigned int axis, int value)
{
    mouseWheelEvent e;

    e.value = value;
    e.backend = Wayland;

    getMainApp()->mouseWheel(e);
}

//keyboard
void waylandAppContext::eventKeyboardKeymap(wl_keyboard *keyboard, unsigned int format, int fd, unsigned int size)
{
    //internal
}

void waylandAppContext::eventKeyboardEnterSurface(wl_keyboard *keyboard, unsigned int serial, wl_surface *surface, wl_array *keys)
{
    if(!surface) return;

    focusEvent e;
    e.state = focusState::gained;

    e.backend = Wayland;

    void* data = wl_surface_get_user_data(surface);
    if(!data) return;

    waylandWC* con = static_cast<waylandWC*>(data);
    e.handler = &con->getWindow();

    e.data = new waylandEventData(serial);

    getMainApp()->windowFocus(e);
}

void waylandAppContext::eventKeyboardLeaveSurface(wl_keyboard *keyboard, unsigned int serial, wl_surface *surface)
{
    if(!surface) return;

    focusEvent e;
    e.state = focusState::lost;

    e.backend = Wayland;

    void* data = wl_surface_get_user_data(surface);
    if(!data) return;

    waylandWC* con = static_cast<waylandWC*>(data);
    e.handler = &con->getWindow();

    e.data = new waylandEventData(serial);


    getMainApp()->windowFocus(e);
}

void waylandAppContext::eventKeyboardKey(wl_keyboard *keyboard, unsigned int serial, unsigned int time, unsigned int key, unsigned int state)
{
    keyEvent e;

    e.backend = Wayland;

    e.key = waylandToKey(key);
    if(state == 1)e.state = pressState::pressed;
    else if(state == 0)e.state = pressState::released;

    getMainApp()->keyboardKey(e);
}

void waylandAppContext::eventKeyboardModifiers(wl_keyboard *keyboard, unsigned int serial, unsigned int mods_depressed, unsigned int mods_latched, unsigned int mods_locked, unsigned int group)
{
    //Modifiers depressed %d, latched %d, locked %d, group %d\n", mods_depressed, mods_latched, mods_locked, group
}

void waylandAppContext::eventWindowResized(wl_shell_surface* shellSurface, unsigned int edges, unsigned int width, unsigned int height)
{
    sizeEvent e;

    e.backend = Wayland;

    e.size = vec2ui(width, height);
    waylandWindowContext* w = static_cast<waylandWindowContext*> (wl_shell_surface_get_user_data(shellSurface));

    if(!w)
        return; //todo: warning or ciritcal error?

    e.handler = &w->getWindow();

    getMainApp()->windowSize(e);
}

void waylandAppContext::setCursor(std::string curse, unsigned int serial)
{
    wl_cursor* curs =  wl_cursor_theme_get_cursor(wlCursorTheme_, curse.c_str());

    if(!curs) return;

    wl_buffer* buffer;
    wl_cursor_image* image;

    image = curs->images[0];
    buffer = wl_cursor_image_get_buffer(image);

    if(serial != 0) wl_pointer_set_cursor(wlPointer_, serial, wlCursorSurface_, image->hotspot_x, image->hotspot_y);
    wl_surface_attach(wlCursorSurface_, buffer, 0, 0);
    wl_surface_damage(wlCursorSurface_, 0, 0, image->width, image->height);
    wl_surface_commit(wlCursorSurface_);
}

void waylandAppContext::setCursor(image* img, unsigned int serial)
{
    /*
    //todo: implement

    wl_buffer buffer = wl_cursor_image_get_buffer(image);
    if(serial != 0) wl_pointer_set_cursor(wlPointer_, serial, m_cursorSurface, image.hotspot_x, image.hotspot_y);
    wl_surface_attach(m_cursorSurface, buffer, 0, 0);
    wl_surface_damage(m_cursorSurface, 0, 0, image.width, image.height);
    wl_surface_commit(m_cursorSurface);
    */
}

#ifdef WithGL
//waylandEGLContext
waylandEGLAppContext::waylandEGLAppContext(waylandAppContext* ac) : appContext_(ac)
{
}

waylandEGLAppContext::~waylandEGLAppContext()
{
}

bool waylandEGLAppContext::init()
{
    //eglBindAPI(EGL_OPENGL_API);

    EGLint major, minor, count, n, size;
    EGLConfig *configs;

    EGLint config_attribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    const EGLint context_attribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };


    eglDisplay_ = eglGetDisplay((EGLNativeDisplayType) appContext_->getWlDisplay());

    if (eglDisplay_ == EGL_NO_DISPLAY)
    {
        std::cout << "Can't create egl display" << std::endl;
        return 0;
    }

    if (eglInitialize(eglDisplay_, &major, &minor) != EGL_TRUE)
    {
        std::cout << "Can't initialise egl display" << std::endl;
        return 0;
    }
    //printf("EGL major: %d, minor %d\n", major, minor);

    eglGetConfigs(eglDisplay_, nullptr, 0, &count);
    //printf("EGL has %d configs\n", count);

    configs = new EGLConfig;

    eglChooseConfig(eglDisplay_, config_attribs, configs, count, &n);

    for (int i = 0; i < n; i++)
    {
        eglGetConfigAttrib(eglDisplay_, configs[i], EGL_BUFFER_SIZE, &size);
        eglGetConfigAttrib(eglDisplay_, configs[i], EGL_RED_SIZE, &size);

        // just choose the first one
        eglConfig_ = configs[i];
        break;
    }

    eglContext_ = eglCreateContext(eglDisplay_, eglConfig_, EGL_NO_CONTEXT, context_attribs);
    //eglBindAPI(EGL_OPENGL_API);
    return 1;
}
#endif // WithGL


}
