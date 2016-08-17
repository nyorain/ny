#include <ny/backend/wayland/egl.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/wayland/util.hpp>

#include <wayland-egl.h>
#include <EGL/egl.h>

namespace ny
{

waylandEGLAppContext::waylandEGLAppContext(waylandAppContext* ac)
{
    eglBindAPI(EGL_OPENGL_API);

    EGLint major, minor, configSize;

    EGLint renderable = EGL_OPENGL_BIT; //todo
    EGLint config_attribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, renderable,
        EGL_NONE
    };

    eglDisplay_ = eglGetDisplay((EGLNativeDisplayType) ac->getWlDisplay());

    if (eglDisplay_ == EGL_NO_DISPLAY)
    {
        throw std::runtime_error("Can't create egl display");
        return;
    }

    if (eglInitialize(eglDisplay_, &major, &minor) != EGL_TRUE)
    {
        throw std::runtime_error("Can't initialise egl display");
        return;
    }

    eglChooseConfig(eglDisplay_, config_attribs, &eglConfig_, 1, &configSize); //todo

    if(!eglConfig_)
    {
        throw std::runtime_error("Can't choose egl config");
        return;
    }

/*
    for (int i = 0; i < n; i++)
    {
        eglGetConfigAttrib(eglDisplay_, configs[i], EGL_BUFFER_SIZE, &size);
        eglGetConfigAttrib(eglDisplay_, configs[i], EGL_RED_SIZE, &size);

        // just choose the first one
        eglConfig_ = configs[i];
        break;
    }
*/
}

waylandEGLAppContext::~waylandEGLAppContext()
{
}

//waylandEGLContext
waylandEGLDrawContext::waylandEGLDrawContext(const waylandWindowContext& wc) : eglDrawContext(wc.getWindow())
{
}

waylandEGLDrawContext::~waylandEGLDrawContext()
{
    eglAppContext* egl = getEGLAppContext();

    if(wlEGLWindow_) wl_egl_window_destroy(wlEGLWindow_);
    if(eglSurface_) eglDestroySurface(egl->getDisplay(), eglSurface_);
}

void waylandEGLDrawContext::initEGL(const waylandWindowContext& wc)
{
    if(wlEGLWindow_)
        return;

    eglBindAPI(EGL_OPENGL_API);

    eglAppContext* egl = getEGLAppContext();

    wlEGLWindow_ = wl_egl_window_create(wc.getWlSurface(), wc.getWindow().getWidth(), wc.getWindow().getHeight());
    if(wlEGLWindow_ == EGL_NO_SURFACE)
    {
        throw std::runtime_error("waylandEglContext::waylandEglContext: wl_egl_window_create failed");
        return;
    }

    eglSurface_ = eglCreateWindowSurface(egl->getDisplay(), egl->getConfig(), (EGLNativeWindowType) wlEGLWindow_, nullptr);
    if(eglSurface_ == EGL_NO_SURFACE)
    {
        throw std::runtime_error("waylandEglContext::waylandEglContext: eglCreateWindowSurface failed");
        return;
    }

    const EGLint contextAttribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    eglContext_ = eglCreateContext(egl->getDisplay(), egl->getConfig(), EGL_NO_CONTEXT, contextAttribs);
    eglDrawContext::init(glApi::openGL);

}

void waylandEGLDrawContext::setSize(Vec2ui size)
{
    if(wlEGLWindow_) wl_egl_window_resize(wlEGLWindow_, size.x, size.y, 0, 0);

    //makeCurrent();
    //glViewport(0, 0, size.x, size.y);
    //makeNotCurrent();
}

//
WaylandEglWindowContext::WaylandEglWindowContext(WaylandAppContext& ac, 
	const WaylandWindowSettings& settings) : WaylandWindowContext(ac, settings)
{
	auto size = settings.size;
    wlEGLWindow_ = wl_egl_window_create(wlSurface(), size.x, size.y);
    if(!wlEGLWindow_) throw std::runtime_error("waylandEglWC: wl_egl_window_create failed");

	auto eglWindow = static_cast<EGLNativeWindowType>(wlEglWindow_);
    eglSurface_ = eglCreateWindowSurface(egl->getDisplay(), egl->getConfig(), eglWindow, nullptr);
    if(!eglSurface_) throw std::runtime_error("waylandEglWC: eglCreateWindowSurface failed");

    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    eglContext_ = eglCreateContext(egl->getDisplay(), egl->getConfig(), EGL_NO_CONTEXT, contextAttribs);
}

DrawGuard WaylandEglWindowContext::draw()
{
	return {drawContext_};
}

void WaylandEglWindowContext::size(const Vec2ui& size)
{
    wl_egl_window_resize(wlEGLWindow_, size.x, size.y, 0, 0);
}

}

#endif // NY_WithGL
