#include <ny/config.h>

#ifdef NY_WithEGL
#include <ny/gl/egl.hpp>
#include <ny/app.hpp>
#include <ny/appContext.hpp>

#include <EGL/egl.h>

namespace ny
{

eglAppContext* getEGLAppContext()
{
    return getMainApp()->getAppContext()->getEGLAppContext();
}

bool eglContext::makeCurrentImpl()
{
    if(!getEGLContext() || !eglSurface_ || !eglContext_)
        return 0;

    return eglMakeCurrent(getEGLAppContext()->getDisplay(), eglSurface_, eglSurface_,  eglContext_);
}

bool eglContext::makeNotCurrentImpl()
{
    if(!getEGLContext())
        return 0;

    return eglMakeCurrent(getEGLAppContext()->getDisplay(),  EGL_NO_SURFACE,  EGL_NO_SURFACE,  EGL_NO_CONTEXT);
}

bool eglContext::swapBuffers()
{
    if(!getEGLContext() || !eglSurface_)
        return 0;

    return eglSwapBuffers(getEGLAppContext()->getDisplay(), eglSurface_);
}

}

#endif // NY_WithEGL
