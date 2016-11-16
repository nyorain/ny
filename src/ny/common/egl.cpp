// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/common/egl.hpp>
#include <ny/log.hpp>

#include <EGL/egl.h>
#include <stdexcept>
#include <cstring>

namespace ny
{

namespace
{

///EGL std::error_category
class EglErrorCategory : public std::error_category
{
public:
	static EglErrorCategory& instance();
	static std::exception exception(nytl::StringParam msg = "");
	static std::error_code errorCode();

public:
	const char* name() const noexcept override { return "ny::EglErrorCategory"; }
	std::string message(int code) const override;
};

}

//EglSetup
///TODO: api loading/library
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

	//query all available configs
	//change this to only hold really required attributes
    constexpr EGLint attribs[] =
	{
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_NONE
    };

	int configSize;
	EGLConfig configs[512] {};

	//it might really fail here on some platforms
    if(!::eglChooseConfig(eglDisplay_, attribs, configs, 512, &configSize))
		throw EglErrorCategory::exception("ny::EglSetup: eglChooseConfig failed");

	if(!configSize)
		throw std::runtime_error("ny::EglSetup: could not retrieve any egl configs");

	configs_.reserve(configSize);
	auto highestRating = 0u;
	for(auto& config : nytl::Range<EGLConfig>(*configs, configSize))
	{
		GlConfig glconf;
		int r, g, b, a, id, depth, stencil, sampleBuffers, samples;

		::eglGetConfigAttrib(eglDisplay_, config, EGL_RED_SIZE, &r);
		::eglGetConfigAttrib(eglDisplay_, config, EGL_GREEN_SIZE, &g);
		::eglGetConfigAttrib(eglDisplay_, config, EGL_BLUE_SIZE, &b);
		::eglGetConfigAttrib(eglDisplay_, config, EGL_ALPHA_SIZE, &a);
		::eglGetConfigAttrib(eglDisplay_, config, EGL_CONFIG_ID, &id);
		::eglGetConfigAttrib(eglDisplay_, config, EGL_DEPTH_SIZE, &depth);
		::eglGetConfigAttrib(eglDisplay_, config, EGL_STENCIL_SIZE, &stencil);
		::eglGetConfigAttrib(eglDisplay_, config, EGL_SAMPLE_BUFFERS, &sampleBuffers);
		::eglGetConfigAttrib(eglDisplay_, config, EGL_SAMPLES, &samples);

		glconf.depth = depth;
		glconf.stencil = stencil;
		glconf.red = r;
		glconf.green = g;
		glconf.blue = b;
		glconf.alpha = a;
		glconf.id = glConfigId(id);
		glconf.doublebuffer = true; //cannot be queried by config, but should always be possible

		if(sampleBuffers) glconf.samples = samples;

		configs_.push_back(glconf);

		auto rating = rate(configs_.back());
		if(rating > highestRating)
		{
			highestRating = rating;
			defaultConfig_ = &configs_.back(); //configs_ will never be reallocated
		}
	}
}

EglSetup::~EglSetup()
{
	if(eglDisplay_)
	{
		::eglTerminate(eglDisplay_);
		::eglReleaseThread(); //XXX: may be a problem if using multiple EglSetups in one thread
	}
}

EglSetup::EglSetup(EglSetup&& other) noexcept
{
	eglDisplay_ = other.eglDisplay_;
	defaultConfig_ = other.defaultConfig_;

	configs_ = std::move(other.configs_);
	glLibrary_ = std::move(other.glLibrary_);
	glesLibrary_ = std::move(other.glesLibrary_);
	eglLibrary_ = std::move(other.eglLibrary_);

	other.eglDisplay_ = {};
	other.defaultConfig_ = {};
}

EglSetup& EglSetup::operator=(EglSetup&& other) noexcept
{
	if(eglDisplay_) ::eglTerminate(eglDisplay_);

	eglDisplay_ = other.eglDisplay_;
	defaultConfig_ = other.defaultConfig_;

	configs_ = std::move(other.configs_);
	glLibrary_ = std::move(other.glLibrary_);
	glesLibrary_ = std::move(other.glesLibrary_);
	eglLibrary_ = std::move(other.eglLibrary_);

	other.eglDisplay_ = {};
	other.defaultConfig_ = {};

	return *this;
}

std::unique_ptr<GlContext> EglSetup::createContext(const GlContextSettings& settings) const
{
	return std::make_unique<EglContext>(*this, settings);
}

void* EglSetup::procAddr(nytl::StringParam name) const
{
	auto ret = reinterpret_cast<void*>(::eglGetProcAddress(name));
	return ret;
}

EGLConfig EglSetup::eglConfig(GlConfigId id) const
{
	EGLConfig eglConfig {};
	int configCount;
	EGLint configAttribs[] = {EGL_CONFIG_ID, static_cast<int>(glConfigNumber(id)), EGL_NONE};

	if(!::eglChooseConfig(eglDisplay_, configAttribs, &eglConfig, 1, &configCount))
		return nullptr;

	return eglConfig;
}

//EglSurface
EglSurface::EglSurface(EGLDisplay dpy, void* nw, GlConfigId configid, const EglSetup& setup)
	: EglSurface(dpy, nw, configid ? setup.config(configid) : setup.defaultConfig(),
		configid ? setup.eglConfig(configid) : setup.eglConfig(setup.defaultConfig().id))
{
}

EglSurface::EglSurface(EGLDisplay dpy, void* nativeWindow, const GlConfig& config,
	EGLConfig eglConfig) : eglDisplay_(dpy), config_(config)
{
	if(!eglConfig)
		throw GlContextError(GlContextErrc::invalidConfig, "ny::EglSurface");


	auto eglnwindow = (EGLNativeWindowType) nativeWindow;
	eglSurface_ = ::eglCreateWindowSurface(eglDisplay_, eglConfig, eglnwindow, nullptr);
	if(!eglSurface_)
		throw EglErrorCategory::exception("ny::EglSurface: eglCreteWindowSurface");
}

EglSurface::~EglSurface()
{
	if(eglDisplay_ && eglSurface_) ::eglDestroySurface(eglDisplay_, eglSurface_);
}

bool EglSurface::apply(std::error_code& ec) const
{
	if(!::eglSwapBuffers(eglDisplay_, eglSurface_))
	{
		ec = EglErrorCategory::errorCode();
		warning("ny::EglSurface::apply (eglSwapBuffers) failed: ", ec.message());
		return false;
	}

	return true;
}

//EglContext
EglContext::EglContext(const EglSetup& setup, const GlContextSettings& settings)
	: setup_(&setup)
{
	auto major = settings.version.major;
	auto minor = settings.version.minor;

	auto eglDisplay = setup.eglDisplay();

	GlConfig glConfig;
	EGLConfig eglConfig;

	if(settings.config)
	{
		glConfig = setup.config(settings.config);
		eglConfig = setup.eglConfig(settings.config);
	}
	else
	{
		glConfig = setup.defaultConfig();
		eglConfig = setup.eglConfig(glConfig.id);
	}

	if(!eglConfig) throw GlContextError(GlContextErrc::invalidConfig, "ny::EglContext");

	EGLContext eglShareContext = nullptr;
	if(settings.share)
	{
		//do the context have to be associated with the same configs?
		auto shareCtx = dynamic_cast<EglContext*>(settings.share);
		if(!shareCtx)
			throw GlContextError(GlContextErrc::invalidSharedContext, "ny::EglContext");

		eglShareContext = shareCtx->eglContext();
	}

	std::vector<std::pair<unsigned int, unsigned int>> versionPairs;
	std::vector<int> contextAttribs;

	versionPairs.reserve(16);
	contextAttribs.reserve(16);

	if(settings.version.api == GlApi::gles)
	{
		if(major == 0 && minor == 0)
		{
			major = 3;
			minor = 2;
		}

		if(major < 1 || major > 3 || minor > 2)
			throw GlContextError(GlContextErrc::invalidVersion, "ny::EglContext");

		versionPairs = {{3, 2}, {3, 1}, {3, 0}, {2, 0}, {1, 1}, {1, 0}};
		eglBindAPI(EGL_OPENGL_ES_API);
	}
	else if(settings.version.api == GlApi::gl)
	{
		if(major == 0 && minor == 0)
		{
			major = 3;
			minor = 2;
		}

		if(major < 1 || major > 4 || minor > 5)
			throw GlContextError(GlContextErrc::invalidVersion, "ny::EglContext");

		versionPairs = {{4, 5}, {3, 3}, {3, 2}, {3, 1}, {3, 0}, {1, 2}, {1, 0}};
		eglBindAPI(EGL_OPENGL_API);

		//profile
		contextAttribs.push_back(EGL_CONTEXT_OPENGL_PROFILE_MASK);
		if(!settings.compatibility) contextAttribs.push_back(EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT);
		else contextAttribs.push_back(EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT);

		//forward compatible
		contextAttribs.push_back(EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE);
		if(settings.forwardCompatible) contextAttribs.push_back(EGL_TRUE);
		else contextAttribs.push_back(EGL_FALSE);
	}
	else
	{
		throw GlContextError(GlContextErrc::invalidApi, "ny::EglContext");
	}

	//debug
	contextAttribs.push_back(EGL_CONTEXT_OPENGL_DEBUG);
	if(settings.debug) contextAttribs.push_back(EGL_TRUE);
	else contextAttribs.push_back(EGL_FALSE);

	contextAttribs.push_back(EGL_CONTEXT_MAJOR_VERSION);
	contextAttribs.push_back(major);

	contextAttribs.push_back(EGL_CONTEXT_MINOR_VERSION);
	contextAttribs.push_back(minor);

	contextAttribs.push_back(EGL_NONE);

	eglContext_ = ::eglCreateContext(eglDisplay, eglConfig, eglShareContext, contextAttribs.data());

	if(!eglContext_ && ::eglGetError() == EGL_BAD_MATCH && !settings.forceVersion)
	{
		for(const auto& p : versionPairs)
		{
			contextAttribs[contextAttribs.size() - 4] = p.first;
			contextAttribs[contextAttribs.size() - 2] = p.second;
			eglContext_ = ::eglCreateContext(eglDisplay, eglConfig, eglShareContext,
				contextAttribs.data());

			if(!eglContext_ && ::eglGetError() == EGL_BAD_MATCH) continue;
			break;
		}
	}

	if(!eglContext_)
		throw EglErrorCategory::exception("ny::EglContext: eglCreateContext");

	GlContext::initContext(settings.version.api, glConfig, settings.share);
}

EglContext::~EglContext()
{

	if(eglContext_)
	{
		std::error_code ec;
		if(!makeNotCurrent(ec))
			warning("ny::~EglContext: failed to make the context not current: ", ec.message());

		::eglDestroyContext(eglDisplay(), eglContext_);
	}
}

bool EglContext::makeCurrentImpl(const GlSurface& surface, std::error_code& ec)
{
	auto eglSurface = dynamic_cast<const EglSurface*>(&surface)->eglSurface();
	if(!eglMakeCurrent(eglDisplay(), eglSurface, eglSurface, eglContext()))
	{
		ec = EglErrorCategory::errorCode();
		warning("ny::EglContext::makeCurrent (eglMakeCurrent) failed: ", ec.message());
		return false;
	}

	return true;
}

bool EglContext::makeNotCurrentImpl(std::error_code& ec)
{
	if(!::eglMakeCurrent(eglDisplay(), nullptr, nullptr, nullptr))
	{
		ec = EglErrorCategory::errorCode();
		warning("ny::EglContext::makeNotCurrent (eglMakeCurrent) failed: ", ec.message());
		return false;
	}

	return true;
}

bool EglContext::compatible(const GlSurface& surface) const
{
	if(!GlContext::compatible(surface)) return false;

	auto eglSurface = dynamic_cast<const EglSurface*>(&surface);
	return (eglSurface);
}

GlContextExtensions EglContext::contextExtensions() const
{
	int minswap, maxswap;
	auto config = eglConfig();
	::eglGetConfigAttrib(eglDisplay(), config, EGL_MIN_SWAP_INTERVAL, &minswap);
	::eglGetConfigAttrib(eglDisplay(), config, EGL_MAX_SWAP_INTERVAL, &maxswap);

	GlContextExtensions ret;
	if(maxswap > 0) ret |= GlContextExtension::swapControl;
	if(minswap < 0) ret |= GlContextExtension::swapControlTear;

	return ret;
}

bool EglContext::swapInterval(int interval, std::error_code& ec) const
{
	if(!::eglSwapInterval(eglDisplay(), interval))
	{
		ec = EglErrorCategory::errorCode();
		return false;
	}

	return true;
}

EGLConfig EglContext::eglConfig() const
{
	return setup_->eglConfig(config().id);
}

//Error category
EglErrorCategory& EglErrorCategory::instance()
{
	static EglErrorCategory ret;
	return ret;
}

std::string EglErrorCategory::message(int code) const
{
	switch(code)
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

std::exception EglErrorCategory::exception(nytl::StringParam msg)
{
	return std::system_error(errorCode(), msg);
}

std::error_code EglErrorCategory::errorCode()
{
	return {::eglGetError(), instance()};
}

}
