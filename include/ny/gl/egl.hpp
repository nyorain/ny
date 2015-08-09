#pragma once

#include <ny/config.h>


#ifdef NY_WithEGL

#include <ny/include.hpp>
#include <ny/gl/glDrawContext.hpp>

#include <nyutil/nonCopyable.hpp>

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

    eglAppContext();

	static eglAppContext* object_;

public:
    eglAppContext(EGLDisplay dpy, EGLConfig conf);
    virtual ~eglAppContext();

    EGLDisplay getDisplay() const { return eglDisplay_; }
    EGLConfig getConfig() const { return eglConfig_; }

	static eglAppContext* getObject() { return object_; }
};

//egl
class eglDrawContext : public glDrawContext
{
protected:
    EGLContext eglContext_ = nullptr;
    EGLSurface eglSurface_ = nullptr;

    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

    eglDrawContext(surface& s);

public:
    eglDrawContext(surface& s, EGLContext ctx, EGLSurface surf);

    virtual bool swapBuffers() override;

    EGLContext getEGLContext() const { return eglContext_; }
    EGLContext getEGLSurface() const { return eglSurface_; }
};

}
#endif // NY_WithEGL
