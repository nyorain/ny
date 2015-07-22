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

eglDrawContext::eglDrawContext(surface& s) : glDrawContext(s)
{
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
