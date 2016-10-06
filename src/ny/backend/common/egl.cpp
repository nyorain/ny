#include <ny/backend/common/egl.hpp>
#include <ny/base/log.hpp>

#include <EGL/egl.h>
#include <stdexcept>
#include <cstring>

namespace ny
{

//EglContextGuard
EglContextGuard::EglContextGuard(EGLDisplay dpy, EGLConfig config, GlContext::Api api)
	: eglDisplay_(dpy), eglConfig_(config), api_(api)
{
	if(api == GlApi::gles)
	{
		eglBindAPI(EGL_OPENGL_ES_API);
		const int attrib[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
		eglContext_ = eglCreateContext(eglDisplay_, config, nullptr, attrib);
	}
	else if(api == GlApi::gl)
	{
		eglBindAPI(EGL_OPENGL_API);
		eglContext_ = eglCreateContext(eglDisplay_, config, nullptr, nullptr);
	}

	if(!eglContext_)
	{
		auto msg = EglContext::errorMessage(eglGetError());
		throw std::runtime_error("ny::EglContextGuard: failed to create EGLContext: " + msg);
	}
}

EglContextGuard::~EglContextGuard()
{
	if(eglDisplay_ && eglContext_) eglDestroyContext(eglDisplay_, eglContext_);
}

EglContextGuard::EglContextGuard(EglContextGuard&& other) noexcept
{
	std::memcpy(this, &other, sizeof(other));
	std::memset(&other, 0, sizeof(other));
}

EglContextGuard& EglContextGuard::operator=(EglContextGuard&& other) noexcept
{
	if(eglDisplay_ && eglContext_) eglDestroyContext(eglDisplay_, eglContext_);
	std::memcpy(this, &other, sizeof(other));
	std::memset(&other, 0, sizeof(other));
	return *this;
}

//EglContext
EglContext::EglContext(EGLDisplay dpy, EGLContext ctx, EGLConfig conf, GlApi api, EGLSurface surf)
	: eglDisplay_(dpy), eglContext_(ctx), eglConfig_(conf), eglSurface_(surf)
{
	GlContext::initContext(api);
}

EglContext::~EglContext()
{
	makeNotCurrent();
}

void EglContext::eglSurface(EGLSurface surface)
{
	eglSurface_ = surface;
}

bool EglContext::makeCurrentImpl()
{
    if(!eglMakeCurrent(eglDisplay(), eglSurface(), eglSurface(), eglContext()))
    {
        warning("EglContext::current: eglMakeCurrent failed: ", errorMessage(eglGetError()));
        return false;
    }

    return true;
}

bool EglContext::makeNotCurrentImpl()
{
    if(!eglMakeCurrent(eglDisplay(), nullptr, nullptr, nullptr))
    {
        warning("EglContext::notCurrent: eglMakeCurrent failed: ", errorMessage(eglGetError()));
        return false;
    }

    return true;
}

bool EglContext::apply()
{
    if(!isCurrent())
    {
		warning("ny::EglContext::apply: invalid or not current");
        return false;
    }

    if(!eglSwapBuffers(eglDisplay(), eglSurface()))
    {
		warning("eglContext::apply: eglSwapBuffers failed\n\t", errorMessage(eglGetError()));
        return false;
    }

    return true;
}

std::vector<std::string> EglContext::eglExtensions() const
{
	return nytl::split(eglQueryString(eglDisplay(), EGL_EXTENSIONS), ' ');
}

bool EglContext::eglExtensionSupported(const std::string& name) const
{
	for(auto& s : eglExtensions())
		if(s == name) return true;

	return false;
}

//static
std::string EglContext::errorMessage(int error)
{
	switch(error)
	{
		case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
		case EGL_SUCCESS: return "EGL_SUCCESS";
		case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
		case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
		case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
		case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
		case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
		case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
		case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
		case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
		case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
		case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
		case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
		case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
		case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
		default: return "unknown error code";
	}
}

int EglContext::eglErrorWarn()
{
	int error = eglGetError();
	if(error != EGL_SUCCESS) warning("ny::EglContext error: ", errorMessage(error));

	return error;
}

void* EglContext::procAddr(const char* name) const
{
	return reinterpret_cast<void*>(eglGetProcAddress(name));
}

}
