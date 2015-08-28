#include <ny/config.h>

#ifdef NY_WithEGL
#include <ny/gl/egl.hpp>
#include <ny/app.hpp>
#include <ny/appContext.hpp>
#include <ny/error.hpp>

#include <EGL/egl.h>

namespace ny
{

eglAppContext* eglAppContext::global_ = nullptr;

//Ac
eglAppContext::eglAppContext()
{
    if(!global_)
        global_ = this;
    //else error
}

eglAppContext::eglAppContext(EGLDisplay dpy, EGLConfig conf) : eglDisplay_(dpy), eglConfig_(conf)
{
    if(!global_)
        global_ = this;
    //else error
}

eglAppContext::~eglAppContext()
{
    if(global_ == this)
        global_ = nullptr;
    //else error
}

//DC
eglDrawContext::eglDrawContext(surface& s) : glDrawContext(s)
{
}

eglDrawContext::eglDrawContext(surface& s, EGLContext ctx, EGLSurface surf) : glDrawContext(s), eglContext_(ctx), eglSurface_(surf)
{
    init(glApi::openGL);
}

bool eglDrawContext::makeCurrentImpl()
{
    if(!getEGLContext() || !eglSurface_ || !eglContext_)
    {
        nyWarning("eglDrawContext::makeCurrentImpl: invalid");
        return 0;
    }


    return eglMakeCurrent(getEGLAppContext()->getDisplay(), eglSurface_, eglSurface_,  eglContext_);
}

bool eglDrawContext::makeNotCurrentImpl()
{
    if(!getEGLContext())
    {
        nyWarning("eglDrawContext::makeNotCurrentImpl: invalid");
        return 0;
    }

    return eglMakeCurrent(getEGLAppContext()->getDisplay(),  EGL_NO_SURFACE,  EGL_NO_SURFACE,  EGL_NO_CONTEXT);
}

bool eglDrawContext::swapBuffers()
{
    if(!getEGLContext() || !eglSurface_)
    {
        nyWarning("eglDrawContext::swapBuffers: invalid");
        return 0;
    }

    return eglSwapBuffers(getEGLAppContext()->getDisplay(), eglSurface_);
}

}

#endif // NY_WithEGL
