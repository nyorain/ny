// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/common/egl.hpp>
#include <ny/log.hpp>

#include <EGL/egl.h>
#ifndef EGL_VERSION_1_4
	#error "EGL Version 1.4 is required at compile time!"
#endif

#include <stdexcept>
#include <cstring>
#include <mutex>

namespace ny {
namespace {

// specifies whether extended egl createContext functionality can be used
bool hasCreateContext = false;
bool has15 = false;

void loadExtensions(EGLDisplay display)
{
	auto exts = eglQueryString(display, EGL_EXTENSIONS);
	hasCreateContext = glExtensionStringContains(exts, "EGL_KHR_create_context");
}

// needed version 1.5 values
#ifndef EGL_VERSION_1_5
	constexpr auto EGL_CONTEXT_MAJOR_VERSION = 0x3098;
	constexpr auto EGL_CONTEXT_MINOR_VERSION = 0x30FB;
	constexpr auto EGL_CONTEXT_OPENGL_PROFILE_MASK = 0x30FD;
	constexpr auto EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT = 0x00000001;
	constexpr auto EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT = 0x00000002;
	constexpr auto EGL_CONTEXT_OPENGL_DEBUG = 0x31B0;
	constexpr auto EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE = 0x31B1;
#endif

// needed create_context extension values
#ifndef EGL_CONTEXT_FLAGS_KHR
	constexpr auto EGL_CONTEXT_FLAGS_KHR = 0x30FC;
	constexpr auto EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR = 0x00000001;
	constexpr auto EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR = 0x00000002;
#endif

} // anonymous util namespace

// EglSetup
EglSetup::EglSetup(void* nativeDisplay)
{
	// It does not matter if NativeDisplayType here is not the real NativeDisplayType that
	// was passed to this function. If multiple platforms are supported, the egl implementation
	// will treat it as void* anyways.
	eglDisplay_ = ::eglGetDisplay((EGLNativeDisplayType) nativeDisplay);
	if(eglDisplay_ == EGL_NO_DISPLAY)
		throw std::runtime_error("ny::EglSetup: eglGetDisplay failed");

	int major, minor;
	if(!::eglInitialize(eglDisplay_, &major, &minor))
		throw EglErrorCategory::exception("ny::EglSetup: eglInitialize failed");

	if(major < 1 || (major == 1 && minor < 4)) {
		ny_error("::egl::EglSetup()"_src, "only egl version {}.{} supported", major, minor);
		throw std::runtime_error("ny::EglSetup: egl 1.4 not supported");
	}

	has15 = major > 1 || (major == 1 && minor > 4);
	loadExtensions(eglDisplay_);

	// query all available configs
	constexpr EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};

	int configSize;
	EGLConfig configs[512] {};

	if(!::eglChooseConfig(eglDisplay_, attribs, configs, 512, &configSize))
		throw EglErrorCategory::exception("ny::EglSetup: eglChooseConfig failed");

	if(!configSize)
		throw std::runtime_error("ny::EglSetup: could not retrieve any egl configs");

	configs_.reserve(configSize);
	auto bestRating = 0u;

	for(auto& config : nytl::Span<EGLConfig>(*configs, configSize)) {
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
		glconf.id = glConfigID(id);
		glconf.doublebuffer = true; // should always be possible
		glconf.transparent = true; // EGL_TRANSPARENT_TYPE usually wrong

		if(sampleBuffers) glconf.samples = samples;

		configs_.push_back(glconf);

		auto rating = rate(glconf);
		if(rating > bestRating) {
			bestRating = rating;
			defaultConfig_ = glconf;
		}
	}
}

EglSetup::~EglSetup()
{
	if(eglDisplay_) {
		::eglTerminate(eglDisplay_);
		::eglReleaseThread();
	}
}

EglSetup::EglSetup(EglSetup&& other) noexcept
{
	eglDisplay_ = other.eglDisplay_;
	defaultConfig_ = other.defaultConfig_;
	configs_ = std::move(other.configs_);

	other.eglDisplay_ = {};
	other.defaultConfig_ = {};
}

EglSetup& EglSetup::operator=(EglSetup&& other) noexcept
{
	if(eglDisplay_) ::eglTerminate(eglDisplay_);

	eglDisplay_ = other.eglDisplay_;
	defaultConfig_ = other.defaultConfig_;
	configs_ = std::move(other.configs_);

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
	return reinterpret_cast<void*>(::eglGetProcAddress(name));
}

EGLConfig EglSetup::eglConfig(GlConfigID id) const
{
	EGLConfig eglConfig {};
	int configCount;
	EGLint configAttribs[] = {EGL_CONFIG_ID, static_cast<int>(glConfigNumber(id)), EGL_NONE};

	if(!::eglChooseConfig(eglDisplay_, configAttribs, &eglConfig, 1, &configCount))
		return nullptr;

	return eglConfig;
}

// EglSurface
EglSurface::EglSurface(const EglSetup& setup, void* nw, GlConfigID configid)
	: EglSurface(setup.eglDisplay(), nw,
		configid ? setup.config(configid) : setup.defaultConfig(),
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
	GlContext* context;
	if(isCurrent(&context)) {
		std::error_code ec;
		if(!context->makeNotCurrent(ec))
			ny_warn("~EglSurface"_scope, "failed not make not current: {}", ec.message());
	}

	if(isCurrentInAnyThread())
		ny_error("~EglSurface"_scope, "still current in another thread.");

	if(eglDisplay_ && eglSurface_)
		::eglDestroySurface(eglDisplay_, eglSurface_);
}

bool EglSurface::apply(std::error_code& ec) const
{
	ec.clear();
	if(!::eglSwapBuffers(eglDisplay_, eglSurface_)) {
		ec = EglErrorCategory::errorCode();
		ny_warn("::EglSurface::apply"_src, "eglSwapBuffers failed: {}", ec.message());
		return false;
	}

	return true;
}

// EglContext
EglContext::EglContext(const EglSetup& setup, const GlContextSettings& settings)
	: setup_(&setup)
{
	dlg_source("EglContext()"_scope);

	auto eglDisplay = setup.eglDisplay();
	auto api = settings.api;

	// get config
	GlConfig glConfig;
	EGLConfig eglConfig;

	if(settings.config) {
		glConfig = setup.config(settings.config);
		eglConfig = setup.eglConfig(settings.config);
	} else {
		glConfig = setup.defaultConfig();
		eglConfig = setup.eglConfig(glConfig.id);
	}

	if(!eglConfig)
		throw GlContextError(GlContextErrc::invalidConfig, "ny::EglContext");

	// share
	EGLContext eglShareContext = nullptr;
	if(settings.share) {
		auto shareCtx = dynamic_cast<EglContext*>(settings.share);
		if(!shareCtx)
			throw GlContextError(GlContextErrc::invalidSharedContext, "ny::EglContext");

		eglShareContext = shareCtx->eglContext();
	}

	// context attributes
	auto apiName = 0u;
	if(api == GlApi::gles) apiName = EGL_OPENGL_ES_API;
	else if(api == GlApi::gl) apiName = EGL_OPENGL_API;

	if(api == GlApi::none) {
		if(::eglBindAPI(EGL_OPENGL_API)) api = GlApi::gl;
		else if(::eglBindAPI(EGL_OPENGL_ES_API)) api = GlApi::gles;
		else throw std::runtime_error("ny::EglContext: egl does not support gl or gles");
	} else if(!::eglBindAPI(apiName)) {
		throw std::runtime_error("ny::EglContext: requested api is not supported!");
	}

	// additional flags
	if(hasCreateContext || has15) {
		std::vector<int> attributes;
		attributes.reserve(20);
		std::vector<std::pair<unsigned int, unsigned int>> versionPairs;

		if(api == GlApi::gles) versionPairs = {{3, 2}, {3, 1}, {3, 0}, {2, 0}, {1, 1}, {1, 0}};
		else versionPairs = {{4, 5}, {3, 3}, {3, 2}, {3, 1}, {3, 0}, {1, 2}, {1, 0}};

		// profile
		if(api == GlApi::gl) {
			attributes.push_back(EGL_CONTEXT_OPENGL_PROFILE_MASK);
			if(!settings.compatibility) attributes.push_back(EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT);
			else attributes.push_back(EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT);
		}

		// there are a few differences between egl 1.5. context creation and the
		// create_context extension... *sigh*
		if(has15) {
			// forward compatible
			if(api == GlApi::gl) {
				attributes.push_back(EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE);
				if(settings.forwardCompatible) attributes.push_back(EGL_TRUE);
				else attributes.push_back(EGL_FALSE);
			}

			// debug
			attributes.push_back(EGL_CONTEXT_OPENGL_DEBUG);
			if(settings.debug) attributes.push_back(EGL_TRUE);
			else attributes.push_back(EGL_FALSE);
		} else {
			auto flags = 0u;
			if(settings.debug) flags |= EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
			if(api == GlApi::gl && settings.forwardCompatible)
				flags |= EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR;

			attributes.push_back(EGL_CONTEXT_FLAGS_KHR);
			attributes.push_back(flags);
		}

		// version set later
		attributes.push_back(EGL_CONTEXT_MAJOR_VERSION);
		attributes.push_back(0);
		attributes.push_back(EGL_CONTEXT_MINOR_VERSION);
		attributes.push_back(0);

		attributes.push_back(EGL_NONE);

		::eglGetError();
		for(const auto p : versionPairs) {
			attributes[attributes.size() - 4] = p.first;
			attributes[attributes.size() - 2] = p.second;

			// unset forward compatible for versions < 3.0
			if(p.first < 3 && api == GlApi::gl) {
				if(has15) attributes[3] = EGL_FALSE;
				else attributes[3] &= ~EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR;
			}

			auto attribData = attributes.data();
			eglContext_ = ::eglCreateContext(eglDisplay, eglConfig, eglShareContext, attribData);

			if(eglContext_)
				break;
		}

		if(!eglContext_) {
			auto msg = EglErrorCategory::errorCode().message();
			ny_warn("could not create context for any version: {}", msg);
		}
	} else {
		ny_info("create_context extension and egl 1.5 not available");
	}

	// try (again) if extension is not present or creation failed
	if(!eglContext_) {
		if(api == GlApi::gles) {
			EGLint attribs[3] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
			eglContext_ = ::eglCreateContext(eglDisplay, eglConfig, eglShareContext, attribs);

			if(!eglContext_) {
				attribs[1] = 1;
				eglContext_ = ::eglCreateContext(eglDisplay, eglConfig, eglShareContext, attribs);
			}
		} else {
			auto none = EGL_NONE;
			eglContext_ = ::eglCreateContext(eglDisplay, eglConfig, eglShareContext, &none);
		}

		if(!eglContext_) {
			auto msg = EglErrorCategory::errorCode().message();
			ny_warn("could not create legacy context: {}", msg);
		}
	}

	if(!eglContext_)
		throw std::runtime_error("ny::EglContext: failed to create egl context");

	GlContext::initContext(api, glConfig, settings.share);
}

EglContext::~EglContext()
{
	if(eglContext_) {
		std::error_code ec;
		if(!makeNotCurrent(ec))
			ny_warn("~EglContext"_scope, "failed to make context not current: {}", ec.message());

		if(isCurrentInAnyThread())
			ny_error("~EglContext"_scope, "still current in another thread");

		::eglDestroyContext(eglDisplay(), eglContext_);
	}
}

bool EglContext::makeCurrentImpl(const GlSurface& surface, std::error_code& ec)
{
	ec.clear();

	auto eglSurface = dynamic_cast<const EglSurface*>(&surface)->eglSurface();
	if(!eglMakeCurrent(eglDisplay(), eglSurface, eglSurface, eglContext())) {
		ec = EglErrorCategory::errorCode();
		ny_warn("::EglContext::makeCurrent"_src, "eglMakeCurrent failed: {}", ec.message());
		return false;
	}

	return true;
}

bool EglContext::makeNotCurrentImpl(std::error_code& ec)
{
	ec.clear();

	if(!::eglMakeCurrent(eglDisplay(), nullptr, nullptr, nullptr)) {
		ec = EglErrorCategory::errorCode();
		ny_warn("::EglContext::makeNotCurrent"_src, "eglMakeCurrent failed: {}", ec.message());
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
	if(!isCurrent()) {
		ec = GlContextErrc::contextNotCurrent;
		return false;
	}

	if(!::eglSwapInterval(eglDisplay(), interval)) {
		ec = EglErrorCategory::errorCode();
		return false;
	}

	return true;
}

EGLConfig EglContext::eglConfig() const
{
	return setup_->eglConfig(config().id);
}

// EglErrorCategory
EglErrorCategory& EglErrorCategory::instance()
{
	static EglErrorCategory ret;
	return ret;
}

std::string EglErrorCategory::message(int code) const
{
	switch(code) {
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

std::system_error EglErrorCategory::exception(nytl::StringParam msg)
{
	return std::system_error(errorCode(), msg);
}

std::error_code EglErrorCategory::errorCode()
{
	return {::eglGetError(), instance()};
}

} // namespace ny
