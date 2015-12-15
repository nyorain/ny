#pragma once

#include <ny/config.h>

#include <ny/draw/include.hpp>
#include <ny/draw/gl/glContext.hpp>

#include <string>
#include <vector>

typedef void* EGLConfig;
typedef void* EGLContext;
typedef void* EGLDisplay;
typedef void* EGLSurface;

namespace ny
{

///EGL implementation for a gl(es) context.
class EglContext : public GlContext
{
public:
	static int eglError();
	static std::string errorMessage(int error);
	static int eglErrorWarn();

protected:
	EGLDisplay eglDisplay_ = nullptr;
    EGLContext eglContext_ = nullptr;
    EGLSurface eglSurface_ = nullptr;
	EGLConfig eglConfig_ = nullptr;

    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

	EglContext() = default;
	void initEglContext(Api api = Api::openGL);

public:
	EglContext(EGLDisplay disp, EGLSurface surf, Api api = Api::openGL);
	EglContext(EGLDisplay disp, EGLConfig config, EGLSurface surf, Api api = Api::openGL);
	virtual ~EglContext();

	EGLDisplay eglDisplay() const { return eglDisplay_; }
    EGLContext eglContext() const { return eglContext_; }
    EGLSurface eglSurface() const { return eglSurface_; }
    EGLConfig eglConfig() const { return eglConfig_; }

	void eglSurface(EGLSurface surface);

	std::vector<std::string> eglExtensions() const;
	bool eglExtensionSupported(const std::string& name) const;

	virtual bool apply() override;
	virtual bool valid() const override;

};

}
