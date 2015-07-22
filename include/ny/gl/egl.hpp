#pragma once

#include <ny/config.h>


#ifdef NY_WithEGL

#include <ny/include.hpp>
#include <ny/gl/glContext.hpp>

#include <ny/util/nonCopyable.hpp>

typedef void* EGLConfig;
typedef void* EGLContext;
typedef void* EGLDisplay;
typedef void* EGLSurface;

namespace ny
{

//eglApp
class eglAppContext : public nonCopyable
{
protected:
    EGLDisplay eglDisplay_ = nullptr;
    EGLConfig eglConfig_ = nullptr;

public:
    EGLDisplay getDisplay() const { return eglDisplay_; }
    EGLConfig getConfig() const { return eglConfig_; }
};

//egl
class eglContext : public glContext
{
protected:
    EGLContext eglContext_ = nullptr;
    EGLSurface eglSurface_ = nullptr;

    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

public:
    virtual bool swapBuffers();

    EGLContext getEGLContext() const { return eglContext_; }
    EGLContext getEGLSurface() const { return eglSurface_; }
};

}
#endif // NY_WithEGL
