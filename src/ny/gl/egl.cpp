#include <ny/config.h>

#ifdef NY_WithEGL
#include <ny/gl/egl.hpp>
#include <ny/app.hpp>
#include <ny/appContext.hpp>

#include <EGL/egl.h>

namespace ny
{

eglAppContext* eglAppContext::object_ = nullptr;

//Ac
eglAppContext::eglAppContext()
{
    if(!object_)
        object_ = this;
    //else error
}

eglAppContext::eglAppContext(EGLDisplay dpy, EGLConfig conf) : eglDisplay_(dpy), eglConfig_(conf)
{
    if(!object_)
        object_ = this;
    //else error
}

eglAppContext::~eglAppContext()
{
    if(object_ == this)
        object_ = nullptr;
    //else error
}

eglAppContext* getEGLAppContext()
{
    return eglAppContext::getObject();
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
        return 0;

    return eglMakeCurrent(getEGLAppContext()->getDisplay(), eglSurface_, eglSurface_,  eglContext_);
}

bool eglDrawContext::makeNotCurrentImpl()
{
    if(!getEGLContext())
        return 0;

    return eglMakeCurrent(getEGLAppContext()->getDisplay(),  EGL_NO_SURFACE,  EGL_NO_SURFACE,  EGL_NO_CONTEXT);
}

bool eglDrawContext::swapBuffers()
{
    if(!getEGLContext() || !eglSurface_)
        return 0;

    return eglSwapBuffers(getEGLAppContext()->getDisplay(), eglSurface_);
}

}

#endif // NY_WithEGL
