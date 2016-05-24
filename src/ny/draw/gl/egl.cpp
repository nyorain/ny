#include <ny/draw/gl/egl.hpp>
#include <ny/base/log.hpp>

#include <EGL/egl.h>

namespace ny
{

//EglContext
EglContext::EglContext(EGLDisplay disp, EGLConfig config, EGLSurface surf, GlContext::Api api)
	: GlContext(), eglDisplay_(disp), eglSurface_(surf), eglConfig_(config)
{
	initEglContext(api);
}

EglContext::EglContext(EGLDisplay disp, EGLSurface surf, GlContext::Api api)
	: GlContext(), eglDisplay_(disp), eglSurface_(surf)
{
	int attribs[] = {EGL_NONE};

	int number;
	eglChooseConfig(eglDisplay_, attribs, &eglConfig_, 1, &number);
	if(number != 0)
	{
		throw std::runtime_error("EglContext::EglContext: failed to choose EGLConfig\n\t" +
				errorMessage(eglError()));
	}

	initEglContext(api);
}

EglContext::~EglContext()
{
	if(eglDisplay_ && eglContext_)
	{
		eglDestroyContext(eglDisplay_, eglContext_);
	}
}

void EglContext::initEglContext(Api api)
{
	api_ = api;

	if(api_ == Api::gles)
	{
		eglBindAPI(EGL_OPENGL_ES_API);
		const int attrib[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
		eglContext_ = eglCreateContext(eglDisplay_, eglConfig_, nullptr, attrib);
	}
	else if(api_ == Api::gl)
	{
		eglBindAPI(EGL_OPENGL_API);
		eglContext_ = eglCreateContext(eglDisplay_, eglConfig_, nullptr, nullptr);
	}

	if(!eglContext_)
	{
		//TODO: throw here? make the function bool?
		throw std::runtime_error("EglContext::EglContext: failed to create EGLContext\n\t" +
				errorMessage(eglError()));
	}

	initContext(api);
}

void EglContext::eglSurface(EGLSurface surface)
{
	//TODO: problematic when current?
	eglSurface_ = surface;
}

bool EglContext::makeCurrentImpl()
{
    if(!valid())
    {
        sendWarning("eglContext::makeCurrentImpl: invalid");
        return 0;
    }

	if(!eglSurface_)
	{
		sendWarning("EglContext::makeCurrentImpl: no egl surface. Trying to make a "
				"context current without surface - may fail");
	}

    if(!eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_))
    {
        sendWarning("eglContext::makeCurrentImpl: eglMakeCurrent failed\n\t",
				errorMessage(eglError()));
        return 0;
    }

    return 1;
}

bool EglContext::makeNotCurrentImpl()
{
    if(!valid())
    {
        sendWarning("eglContext::makeNotCurrentImpl: invalid");
        return 0;
    }

    if(!eglMakeCurrent(eglDisplay_, nullptr, nullptr, nullptr))
    {
        sendWarning("eglContext::makeNotCurrentImpl: eglMakeCurrent failed\n\t",
				errorMessage(eglError()));
        return 0;
    }

    return 1;
}

bool EglContext::apply()
{
    if(!isCurrent() || !valid())
    {
		sendWarning("eglContext::apply: invalid or not current");
        return 0;
    }

	//TODO: single buffered?
    if(!eglSwapBuffers(eglDisplay_, eglSurface_))
    {
		sendWarning("eglContext::apply: eglSwapBuffers failed\n\t",
				errorMessage(eglError()));
        return 0;
    }

    return 1;
}

bool EglContext::valid() const
{
	return (eglDisplay_ && eglContext_);
}

std::vector<std::string> EglContext::eglExtensions() const
{
	return nytl::split(eglQueryString(eglDisplay_, EGL_EXTENSIONS), ' ');
}

bool EglContext::eglExtensionSupported(const std::string& name) const
{
	for(auto& s : eglExtensions())
		if(s == name) return 1;

	return 0;
}

//static
int EglContext::eglError()
{
	return eglGetError();
}

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
	int error = eglError();
	if(error != EGL_SUCCESS)
	{
		sendWarning("EglContext error: ", errorMessage(error));
	}

	return error;
}

void* EglContext::procAddr(const char* name) const
{
	return eglGetProcAddr(name);
}

}
