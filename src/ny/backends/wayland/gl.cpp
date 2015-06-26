#include <ny/config.h>

#ifdef WithGL

#include "backends/wayland/gl.hpp"
#include "backends/wayland/appContext.hpp"
#include "backends/wayland/utils.hpp"

#include "app/error.hpp"
#include "window/window.hpp"
#include "graphics/gl/glDC.hpp"
#include "graphics/gl/glContext.hpp"

#include <wayland-egl.h>
#include <EGL/egl.h>

namespace ny
{


//waylandGL
waylandGLContext::waylandGLContext(waylandWindowContext& wc)
{
    waylandAppContext* context = wc.getAppContext();

    wlEGLWindow_ = wl_egl_window_create(wc.getWlSurface(), wc.getWindow().getWidth(), wc.getWindow().getHeight());
    if(wlEGLWindow_ == EGL_NO_SURFACE)
    {
        throw error(error::Critical, "could not initialize waylandEGLWindow");
        return;
    }

    eglSurface_ = eglCreateWindowSurface(context->getEGLDisplay(), context->getEGLConfig(), wlEGLWindow_, nullptr);
    if(!eglMakeCurrent(context->getEGLDisplay(), eglSurface_, eglSurface_, context->getEGLContext()))
    {
        throw error(error::Critical, "could not make eglContext for eglSurface current");
        return;
    }

    //drawContext_ = new glDrawContext(wc.getWindow());
}

waylandGLContext::~waylandGLContext()
{
}

void waylandGLContext::glSetSize(vec2ui size)
{
    wl_egl_window_resize(wlEGLWindow_, size.x, size.y, 0, 0);
    //glViewport(0, 0, size.x, size.y);
}

//toplevel
waylandGLToplevelWindowContext::waylandGLToplevelWindowContext(toplevelWindow& win, const waylandWindowContextSettings& s) : windowContext((window&)win, s), waylandToplevelWindowContext(win, s), waylandGLContext((waylandWindowContext&)*this)
{
}

drawContext& waylandGLToplevelWindowContext::beginDraw()
{
    eglMakeCurrent(context_->getEGLDisplay(), eglSurface_, eglSurface_, context_->getEGLContext());
    return *drawContext_;
}

void waylandGLToplevelWindowContext::finishDraw()
{
    drawContext_->apply();

    wlFrameCallback_ = wl_surface_frame(wlSurface_);
    wl_callback_add_listener(wlFrameCallback_, &wayland::frameListener, this);
    wl_surface_commit(wlSurface_);

    eglSwapBuffers(context_->getEGLDisplay(), eglSurface_);

}

void waylandGLToplevelWindowContext::setSize(vec2ui size, bool change)
{
    eglMakeCurrent(context_->getEGLDisplay(), eglSurface_, eglSurface_, context_->getEGLContext());
    glSetSize(size);
    refresh();
}

//child
waylandGLChildWindowContext::waylandGLChildWindowContext(childWindow& win, const waylandWindowContextSettings& s) : windowContext((window&)win, s), waylandChildWindowContext(win, s), waylandGLContext((waylandWindowContext&)*this)
{
}

drawContext& waylandGLChildWindowContext::beginDraw()
{
    eglMakeCurrent(context_->getEGLDisplay(), eglSurface_, eglSurface_, context_->getEGLContext());
    return *drawContext_;
}

void waylandGLChildWindowContext::finishDraw()
{
    drawContext_->apply();

    wlFrameCallback_ = wl_surface_frame(wlSurface_);
    wl_callback_add_listener(wlFrameCallback_, &wayland::frameListener, this);

    eglSwapBuffers(context_->getEGLDisplay(), eglSurface_);
}

void waylandGLChildWindowContext::setSize(vec2ui size, bool change)
{
    eglMakeCurrent(context_->getEGLDisplay(), eglSurface_, eglSurface_, context_->getEGLContext());
    glSetSize(size);
    refresh();
}


}

#endif // WithGL
