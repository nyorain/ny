#pragma once

#include <ny/include.hpp>
#include <ny/backend/common/gl.hpp>

#include <string>
#include <vector>

using EGLDisplay = void*; //Singleton per backend. Connection
using EGLConfig = void*; //Represents a context setup. May have more than one
using EGLContext = void*; //The actual context. May have more than one
using EGLSurface = void*; //One surface needed per WindowContext

namespace ny
{

///RAII wrapper around an EGLContext.
class EglContextGuard
{
public:
	EglContextGuard() = default;
	EglContextGuard(EGLDisplay, EGLConfig config, GlApi api = GlApi::gl);
	~EglContextGuard();

	EglContextGuard(EglContextGuard&& other) noexcept;
	EglContextGuard& operator=(EglContextGuard&& other) noexcept;

	EGLDisplay eglDisplay() const { return eglDisplay_; }
    EGLContext eglContext() const { return eglContext_; }
    EGLConfig eglConfig() const { return eglConfig_; }
	GlApi glApi() const { return api_; }

protected:
	EGLDisplay eglDisplay_;
	EGLContext eglContext_;
	EGLConfig eglConfig_;
	GlApi api_;
};

///EglContext for a specific surface
///Holds a reference to a EglContextGuard and an EGLSurface
class EglContext : public GlContext
{
public:
	///Returns the message associated with a given egl error code.
	static std::string errorMessage(int error);

	///Outputs a warning with the last egl error if there was any.
	static int eglErrorWarn();

public:
	EglContext(const EglContextGuard&, EGLSurface = nullptr);
	virtual ~EglContext();

	///Changes the surface associated with this context, i.e. the surface for which the context
	///will be made current on a call to makeCurrent (makeCurrentImpl).
	///Note that for this call to have an effect the context must be made current (if it
	///already is current it must first be made not current).
	///Calling this between making the context current for a different surface and calling
	///apply() results in undefined behaviour.
	void eglSurface(EGLSurface surface);

	std::vector<std::string> eglExtensions() const;
	bool eglExtensionSupported(const std::string& name) const;

	EGLDisplay eglDisplay() const { return context_->eglDisplay(); }
    EGLContext eglContext() const { return context_->eglContext(); }
    EGLConfig eglConfig() const { return context_->eglConfig(); }
    EGLSurface eglSurface() const { return eglSurface_; }

	virtual bool apply() override;
	virtual void* procAddr(const char* name) const override;

protected:
    virtual bool makeCurrentImpl() override;
    virtual bool makeNotCurrentImpl() override;

protected:
	const EglContextGuard* context_ {};
    EGLSurface eglSurface_ {};
};

}

#ifndef NY_WithEGL
	#error ny was built without egl. Do not include this header.
#endif
