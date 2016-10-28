#pragma once

#include <ny/include.hpp>
#include <ny/backend/common/gl.hpp>
#include <ny/backend/common/library.hpp>

#include <string>
#include <vector>
#include <system_error>

using EGLDisplay = void*; //Singleton per backend. Connection
using EGLConfig = void*; //Represents a context setup. May have more than one
using EGLContext = void*; //The actual context. May have more than one
using EGLSurface = void*; //One surface needed per WindowContext

namespace ny
{

///RAII wrapper around an EGLContext.
class EglContextGuard : public nytl::NonCopyable
{
public:
	EglContextGuard() = default;
	EglContextGuard(EGLDisplay, EGLConfig config, EGLContext shared, GlApi api = GlApi::gl);
	~EglContextGuard();

	EglContextGuard(EglContextGuard&& other) noexcept;
	EglContextGuard& operator=(EglContextGuard&& other) noexcept;

	EGLDisplay eglDisplay() const { return eglDisplay_; }
	EGLContext eglContext() const { return eglContext_; }
	EGLConfig eglConfig() const { return eglConfig_; }
	GlApi glApi() const { return api_; }
	bool shared() const { return shared_; }

protected:
	EGLDisplay eglDisplay_ {};
	EGLContext eglContext_ {};
	EGLConfig eglConfig_ {};
	bool shared_ {};
	GlApi api_;
};

//TODO: replace void* as display with some NativeHandle class
//TODO: dont select config here (but in EglContextGuard).
//TODO: better documentation (see winapi/wgl WglSetup)
///Manages multiple egl contexts and the loading of egl extensions functions.
class EglSetup : public nytl::NonCopyable
{
public:
	EglSetup(void* natvieDisplay);
	~EglSetup();

	///Creates a shared or unique context.
	EGLContext createContext(bool& shared, bool unique = true);

	///Returns a shared context that may also be used by other EglContext objects.
	EGLContext getContext(bool& shared);

	EGLDisplay eglDisplay() const { return eglDisplay_; }
	EGLConfig eglConfig() const { return eglConfig_; }

	std::vector<EGLContext> sharedContexts() const;
	EGLContext sharedContext() const;

	void* procAddr(nytl::StringParam& name) const;

protected:
	std::vector<EglContextGuard> shared_;
	std::vector<EglContextGuard> unique_;
	
	Library eglLibrary_;

	EGLDisplay eglDisplay_ {};
	EGLConfig eglConfig_ {};
};

///EglContext for a specific surface
///Holds basically just an EGLContext, its EGLConfig and an EGLSurface.
///Note that this class does not own the EGLContext it holds.
class EglContext : public GlContext
{
public:
	///Returns the message associated with a given egl error code.
	static const char* errorMessage(int error);

	///Outputs a warning with the last egl error if there was any.
	static int eglErrorWarn();

public:
	EglContext(EGLDisplay, EGLContext, EGLConfig, GlApi = GlApi::gl, EGLSurface = nullptr);
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

	EGLDisplay eglDisplay() const { return eglDisplay_; }
	EGLContext eglContext() const { return eglContext_; }
	EGLConfig eglConfig() const { return eglConfig_; }
	EGLSurface eglSurface() const { return eglSurface_; }

	bool apply(std::error_code& ec) override;
	void* procAddr(nytl::StringParam name) const override;
	void* nativeHandle() const override { return static_cast<void*>(eglContext_); }

protected:
	bool makeCurrentImpl(std::error_code& ec) override;
	bool makeNotCurrentImpl(std::error_code& ec) override;

protected:
	EGLDisplay eglDisplay_ {};
	EGLContext eglContext_ {};
	EGLConfig eglConfig_ {};
	EGLSurface eglSurface_ {};
};

///EGL std::error_category
class EglErrorCategory : public std::error_category
{
public:
	static EglErrorCategory& instance();
	static std::exception exception(nytl::StringParam msg = "");

public:
	const char* name() const noexcept override { return "ny::EglErrorCategory"; }
	std::string message(int code) const override;
};

}

#ifndef NY_WithEGL
	#error ny was built without egl. Do not include this header.
#endif
