#include <ny/backend/common/egl.hpp>
#include <ny/base/log.hpp>

#include <EGL/egl.h>
#include <stdexcept>
#include <cstring>

namespace ny
{

//EglContextGuard
EglContextGuard::EglContextGuard(EGLDisplay dpy, EGLConfig config, EGLContext shared, 
	GlContext::Api api) : eglDisplay_(dpy), eglConfig_(config), api_(api)
{
	if(api == GlApi::gles)
	{
		eglBindAPI(EGL_OPENGL_ES_API);
		int attribs[] = 
		{
			EGL_CONTEXT_MAJOR_VERSION, 3, 
			EGL_CONTEXT_MINOR_VERSION, 2,
			0
		};

		unsigned int versionPairs[][2] = {{3, 1}, {3, 0}, {2, 0}, {1, 1}, {1, 0}};
		for(const auto& p : versionPairs)
		{
			eglContext_ = eglCreateContext(eglDisplay_, config, shared, attribs);
			if(!eglContext_)
			{
				attribs[1] = p[0];
				attribs[3] = p[1];
				continue;
			}

			break;
		}
	}
	else if(api == GlApi::gl)
	{
		eglBindAPI(EGL_OPENGL_API);

		//We always ask for the core profile and try to create
		//the context with a gl version as high as possible
		//TODO: core/compat switch
		//TODO: forward compatible switch
		//TODO: debug switch
		int attribs[] = 
		{
			EGL_CONTEXT_MAJOR_VERSION, 4, 
			EGL_CONTEXT_MINOR_VERSION, 5,
			EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
			0
		};

		unsigned int versionPairs[][2] = {{3, 3}, {3, 2}, {3, 1}, {3, 0}, {1, 2}, {1, 0}};
		for(const auto& p : versionPairs)
		{
			eglContext_ = eglCreateContext(eglDisplay_, config, shared, attribs);
			if(!eglContext_)
			{
				attribs[1] = p[0];
				attribs[3] = p[1];
				continue;
			}

			break;
		}
	}
	else
	{
		throw std::logic_error("ny::EglContextGuard: invalid GlApi enum value");
	}

	if(!eglContext_)
		throw EglErrorCategory::exception("ny::EglContextGuard: eglCreateContext failed");

	if(shared) shared_ = true;
	else shared_ = false;
}

EglContextGuard::~EglContextGuard()
{
	if(eglDisplay_ && eglContext_) 
	{
		if(::eglGetCurrentContext() == eglContext_) 
			::eglMakeCurrent(eglDisplay_, nullptr, nullptr, nullptr);

		::eglDestroyContext(eglDisplay_, eglContext_);
	}
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

//EglSetup
EglSetup::EglSetup(void* nativeDisplay)
{
	//It does not matter if NativeDisplayType here is not the real NativeDisplayType that
	//was passed to this function. If multiple platforms are supported, the egl implementation
	//will treat it as void* anyways.
	eglDisplay_ = ::eglGetDisplay((EGLNativeDisplayType) nativeDisplay);
    if(eglDisplay_ == EGL_NO_DISPLAY)
        throw std::runtime_error("ny::EglSetup: eglGetDisplay failed");

	int major, minor;
	if(!::eglInitialize(eglDisplay_, &major, &minor))
		throw EglErrorCategory::exception("ny::EglSetup: eglInitialize failed");
	
	log("ny::EglSetup: egl version: ", major, ".", minor);

    EGLint renderable = EGL_OPENGL_BIT; //todo
    EGLint attribs[] = 
	{
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, renderable,
        EGL_NONE
    };

	int configSize;

	//TODO: better config choosing... !important
	//it might really fail here on some platforms
    ::eglChooseConfig(eglDisplay_, attribs, &eglConfig_, 1, &configSize);
    if(!eglConfig_) throw EglErrorCategory::exception("ny::EglSetup: eglChooseConfig failed");
}

EglSetup::~EglSetup()
{
	if(eglDisplay_)
	{
		::eglTerminate(eglDisplay_);
		::eglReleaseThread(); //XXX: may be a problem if using multiple EglDisplays in one thread
	}
}

EGLContext EglSetup::createContext(bool& shared, bool unique)
{
}

EGLContext EglSetup::getContext(bool& shared)
{
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

bool EglContext::makeCurrentImpl(std::error_code& ec)
{
	if(!eglMakeCurrent(eglDisplay(), eglSurface(), eglSurface(), eglContext()))
	{
		auto error = eglGetError();
		warning("ny::EglContext::makeCurrent (eglMakeCurrent) failed: ", errorMessage(error));
		ec = {error, EglErrorCategory::instance()};
		return false;
	}

	return true;
}

bool EglContext::makeNotCurrentImpl(std::error_code& ec)
{
	if(!eglMakeCurrent(eglDisplay(), nullptr, nullptr, nullptr))
	{
		auto error = eglGetError();
		warning("ny::EglContext::makeNotCurrent (eglMakeCurrent) failed: ", errorMessage(error));
		ec = {error, EglErrorCategory::instance()};
		return false;
	}

	return true;
}

bool EglContext::apply(std::error_code& ec)
{
	//does the context have to be current?
	if(!eglSwapBuffers(eglDisplay(), eglSurface()))
	{
		auto error = eglGetError();
		warning("ny::EglContext::apply (eglSwapBuffers) failed: ", errorMessage(error));
		ec = {error, EglErrorCategory::instance()};
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
const char* EglContext::errorMessage(int error)
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
		default: return "<unknown egl error>";
	}
}

int EglContext::eglErrorWarn()
{
	int error = eglGetError();
	if(error != EGL_SUCCESS) warning("ny::EglContext error: ", errorMessage(error));

	return error;
}

void* EglContext::procAddr(nytl::StringParam name) const
{
	return reinterpret_cast<void*>(eglGetProcAddress(name));
}

//Error category
EglErrorCategory& EglErrorCategory::instance()
{
	static EglErrorCategory ret;
	return ret;
}

std::string EglErrorCategory::message(int code) const
{
	return EglContext::errorMessage(code);
}

std::exception EglErrorCategory::exception(nytl::StringParam msg)
{
	return std::system_error(::eglGetError(), instance(), msg);
}

}
