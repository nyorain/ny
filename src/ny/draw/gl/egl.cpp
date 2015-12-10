#include <ny/config.h>

#ifdef NY_WithEGL
#include <ny/gl/egl.hpp>
#include <ny/app.hpp>
#include <ny/appContext.hpp>
#include <ny/error.hpp>

#include <EGL/egl.h>

namespace ny
{

//EglContext
EglContext::EglContext(EGLDisplay disp, EGLConfig config, EGLSurface surf, GlContext::Api api) 
	: GlContext(), display_(disp), surface_(surf)
{
	init(config, api);
}

EglContext::EglContext(EGLDisplay disp, EGLSurface surf, GlContext::Api api) 
	: GlContext(), display_(disp), surface_(surf)
{
	EGLConfig config;
	int attribs[] = {EGL_NONE};

	int number;
	eglChooseConfig(eglDisplay_, attribs, &config, 1, &number);
	if(number != 0)
	{
		throw std::runtime_error("EglContext::EglContext: failed to choose EGLConfig\n\t",
				eglErrorMessage(eglError()));
	}

	init(config, api);
}

void EglContext::init(EGLConfig config, Api api)
{
	api_ = api;

	if(api_ == Api::openGLES)
	{
		eglBindApi(EGL_OPENGL_ES_API);
		const int attrib[] = {EGL_CONTEXT_CLIENT_VERSION, 2};
		eglContext_ = eglCreateContext(eglDisplay_, config, nullptr, attrib);
	}
	else if(api_ == Api::openGL)
	{
		eglBindApi(EGL_OPENGL_API);
		eglContext_ = eglCreateContext(eglDisplay_, config, nullptr, nullptr);
	}

	if(!eglContext_)
	{
		//TODO: throw here? make the function bool?
		throw std::runtime_error("EglContext::EglContext: failed to create EGLContext\n\t",
				eglErrorMessage(eglError()));
	}

	initContext();
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
        nyWarning("eglContext::makeCurrentImpl: invalid");
        return 0;
    }

    if(!eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_))
    {
        nyWarning("eglContext::makeCurrentImpl: eglMakeCurrent failed\n\t", 
				eglErrorMessage(eglError()));
        return 0;
    }

    return 1;
}

bool EglContext::makeNotCurrentImpl()
{
    if(!valid())
    {
        nyWarning("eglContext::makeNotCurrentImpl: invalid");
        return 0;
    }

    if(!eglMakeCurrent(eglDisplay_, nullptr, nullptr, nullptr))
    {
        nyWarning("eglContext::makeNotCurrentImpl: eglMakeCurrent failed\n\t",
				eglErrorMessage(eglError()));
        return 0;
    }

    return 1;
}

bool EglContext::apply()
{
    if(!isCurrent() || !valid())
    {
        nyWarning("eglContext::apply: invalid or not current");
        return 0;
    }

	//TODO: single buffered?
    if(!eglSwapBuffers(eglDisplay_, eglSurface_))
    {
        nyWarning("eglContext::apply: eglSwapBuffers failed\n\t",
				eglErrorMessage(eglError()));
        return 0;
    }

    return 1;
}

bool EglContext::valid() const
{
	return (eglDisplay_ && eglContext_ && eglSurface_);
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
		default return "unknown error code";
	}
}

int EglContext::eglErrorWarn()
{
	int error = eglError();
	if(error != EGL_SUCCESS)
	{
		nytl::sendWarning("EglContext error: ", errorMessage(error));
	}

	return error;
}

}

#endif // NY_WithEGL
