#pragma once

#include <ny/include.hpp>
#include <ny/backend/common/gl.hpp>

#include <string>
#include <vector>

using EGLConfig = void*;
using EGLContext = void*;
using EGLDisplay = void*;
using EGLSurface = void*;

namespace ny
{

///EGL implementation for a gl(es) context.
class EglContext : public GlContext
{
public:
	static int eglError();
	static std::string errorMessage(int error);
	static int eglErrorWarn();

public:
	EglContext(EGLDisplay disp, EGLSurface surf, Api api = Api::gl);
	EglContext(EGLDisplay disp, EGLConfig config, EGLSurface surf, Api api = Api::gl);
	virtual ~EglContext();

	EGLDisplay eglDisplay() const { return eglDisplay_; }
    EGLContext eglContext() const { return eglContext_; }
    EGLSurface eglSurface() const { return eglSurface_; }
    EGLConfig eglConfig() const { return eglConfig_; }

	void eglSurface(EGLSurface surface);

	std::vector<std::string> eglExtensions() const;
	bool eglExtensionSupported(const std::string& name) const;

	virtual bool apply() override;

	virtual void* procAddr(const char* name) const override;

protected:
	EGLDisplay eglDisplay_ = nullptr;
    EGLContext eglContext_ = nullptr;
    EGLSurface eglSurface_ = nullptr;
	EGLConfig eglConfig_ = nullptr;

    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

	EglContext() = default;
	void initEglContext(Api api = Api::gl);

};

}

#ifndef NY_WithEGL
	#error ny was built without egl. Do not include this header.
#endif
