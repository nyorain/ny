// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/glx.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/surface.hpp>
#include <ny/log.hpp>

#include <nytl/span.hpp>

#include <xcb/xcb.h>
#include <GL/glx.h>

#include <algorithm>
#include <mutex>

// TODO: this needs massive error (code) handling improvements (see GlContext implementation)
// NOTE: For glx GlConfig objects, the id member is the GLX_FB_CONFIG_ID of the associated
// glx fb config.

namespace ny {
namespace ext {

using PfnGlxSwapIntervalEXT = void (APIENTRYP)(Display*, GLXDrawable, int);
using PfnGlxCreateContextAttribsARB = GLXContext(APIENTRYP)(Display*, GLXFBConfig,
	GLXContext, Bool, const int*);

// extensions function pointers
PfnGlxSwapIntervalEXT swapIntervalEXT;
PfnGlxCreateContextAttribsARB createContextAttribsARB;

// bool flags for extensions without functions
bool hasSwapControlTear = false;
bool hasProfile = false;
bool hasProfileES = false;

bool loadExtensions(Display& dpy)
{
	static std::once_flag cof {};
	static bool valid {};

	std::call_once(cof, [&]{
		// we call both since they are different on some platforms
		auto client = ::glXGetClientString(&dpy, GLX_EXTENSIONS);
		auto exts = ::glXQueryExtensionsString(&dpy, 0);

		if(!client || !exts) {
			warning("ny::Glx::loadExtensions: failed to retrieve extension string");
			return;
		}

		std::string extensions;
		if(client) extensions += client;
		if(exts) extensions += exts;

		auto swapControl = glExtensionStringContains(extensions, "GLX_ARB_swap_control");
		auto createContext = glExtensionStringContains(extensions, "GLX_ARB_create_context");

		hasSwapControlTear = glExtensionStringContains(extensions, "GLX_ARB_swap_control_tear");
		hasProfile = glExtensionStringContains(extensions, "GLX_EXT_create_context_profile");
		hasProfileES = glExtensionStringContains(extensions, "GLX_EXT_create_context_es2_profile");

		using PfnVoid = void (*)();
		struct LoadFunc {
			const char* name;
			PfnVoid& func;
			bool load {true};
		} funcs[] = {
			{"glXCreateContextAttribsARB", (PfnVoid&) createContextAttribsARB, createContext},
			{"glXSwapIntervalEXT", (PfnVoid&) swapIntervalEXT, swapControl},
		};

		for(const auto& f : funcs) {
			auto data = reinterpret_cast<const unsigned char*>(f.name);
			f.func = f.load ? reinterpret_cast<PfnVoid>(::glXGetProcAddress(data)) : nullptr;
		}

		valid = true;
	});

	return valid;
}

// TODO: use constexpr
#define GLX_CONTEXT_DEBUG_BIT_ARB 0x00000001
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
#define GLX_CONTEXT_FLAGS_ARB 0x2094
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define GLX_CONTEXT_PROFILE_MASK_ARB 0x9126
#define GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20B2
#define GLX_SAMPLE_BUFFERS_ARB 100000
#define GLX_SAMPLES_ARB 100001
#define GLX_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004
#define GLX_CONTEXT_ES_PROFILE_BIT_EXT 0x00000004
#define GLX_SWAP_INTERVAL_EXT 0x20F1
#define GLX_MAX_SWAP_INTERVAL_EXT 0x20F2

} // namespace ext

namespace {

// error code used to signal invalid api versions
constexpr auto glxBadFBConfig = 178u;
thread_local int lastErrorCode;

// This error handler might be signaled when glx context creation fails
// We will test various gl api versions, so we install this handler that
// the application does not end for the expected error
// Will output GlxBadFBConfig for api version testings
// otherwise a "real" (i.e. unexpected) error might ocurr,
// context creation is generally rather error prone
int ctxErrorHandler(Display* display, XErrorEvent* event)
{
	if(!display || !event) {
		warning("ny::GlxContext: xlib error handler called with nullptr");
		return 0;
	}

	// we expect the glxBadFBConfig error since we test the various supported
	// gl api versions which will produce this error if not supported.
	auto code = static_cast<int>(event->error_code);
	if(code != glxBadFBConfig) {
		auto msg = x11::errorMessage(*display, code);
		warning("ny::GlxConext: unexpected error durin context creation: ", code, ": ", msg);
	}

	lastErrorCode = code;
	return 0;
}

} // anonymous util namespace

// GlxSetup
GlxSetup::GlxSetup(const X11AppContext& ac, unsigned int screenNum) : xDisplay_(&ac.xDisplay())
{
	// query version
	// we need at least glx 1.3 for fb configs
	// should be supported on almost all machines
	int major, minor;
	::glXQueryVersion(xDisplay_, &major, &minor);
	if(major < 1 || (major == 1 && minor < 3))
		throw std::runtime_error("ny::GlxSetup: glx 1.3 not supported");

	if(!ext::loadExtensions(*xDisplay()))
		throw std::runtime_error("ny::GlxSetup: failed to load extensions");

	int fbcount = 0;
	GLXFBConfig* fbconfigs = ::glXGetFBConfigs(xDisplay_, screenNum, &fbcount);
	if(!fbconfigs || !fbcount)
		throw std::runtime_error("ny::GlxSetup: could not retrieve any fb configs");

	configs_.reserve(fbcount);
	auto highestRating = 0u;
	auto highestTransparentRating = 0u;

	for(auto& config : nytl::Span<GLXFBConfig>(*fbconfigs, fbcount)) {
		// TODO: transparency: also query/handle GLX_TRANSPARENT_TYPE?
		// in addition to 32 bit visual querying
		// was invalid on tested platforms though...

		GlConfig glconf;
		int r, g, b, a, id, depth, stencil, doubleBuffer, visual;

		::glXGetFBConfigAttrib(xDisplay_, config, GLX_FBCONFIG_ID, &id);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_STENCIL_SIZE, &stencil);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_DEPTH_SIZE, &depth);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_DOUBLEBUFFER, &doubleBuffer);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_VISUAL_ID, &visual);

		::glXGetFBConfigAttrib(xDisplay_, config, GLX_RED_SIZE, &r);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_GREEN_SIZE, &g);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_BLUE_SIZE, &b);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_ALPHA_SIZE, &a);

		auto visual32 = (x11::visualDepth(ac.xDefaultScreen(), visual) == 32);

		glconf.depth = depth;
		glconf.stencil = stencil;
		glconf.red = r;
		glconf.green = g;
		glconf.blue = b;
		glconf.alpha = a;
		glconf.id = glConfigID(id);
		glconf.doublebuffer = doubleBuffer;
		glconf.transparent = visual32;

		configs_.push_back(glconf);
		auto rating = rate(glconf);
		if(rating > highestRating) {
			highestRating = rating;
			defaultConfig_ = glconf;
		}

		if(glconf.transparent && rating > highestTransparentRating) {
			highestTransparentRating = rating;
			defaultTransparentConfig_ = glconf;
		}
	}

	::XFree(fbconfigs);
}

GlxSetup::GlxSetup(GlxSetup&& other) noexcept
{
	xDisplay_ = other.xDisplay_;
	defaultConfig_ = other.defaultConfig_;
	defaultTransparentConfig_ = other.defaultTransparentConfig_;
	configs_ = std::move(other.configs_);

	other.xDisplay_ = {};
	other.defaultConfig_ = {};
}

GlxSetup& GlxSetup::operator=(GlxSetup&& other) noexcept
{
	xDisplay_ = other.xDisplay_;
	defaultConfig_ = other.defaultConfig_;
	defaultTransparentConfig_ = other.defaultTransparentConfig_;
	configs_ = std::move(other.configs_);

	other.xDisplay_ = {};
	other.defaultConfig_ = {};

	return *this;
}

std::unique_ptr<GlContext> GlxSetup::createContext(const GlContextSettings& settings) const
{
	return std::make_unique<GlxContext>(*this, settings);
}

void* GlxSetup::procAddr(nytl::StringParam name) const
{
	auto data = reinterpret_cast<const unsigned char*>(name.data());
	auto ret = reinterpret_cast<void*>(::glXGetProcAddress(data));
	return ret;
}

GLXFBConfig GlxSetup::glxConfig(GlConfigID id) const
{
	int fbcount {};
	auto* fbconfigs = ::glXGetFBConfigs(xDisplay_, 0, &fbcount);
	if(!fbconfigs || !fbcount) return nullptr;

	GLXFBConfig ret {};
	for(auto& config : nytl::Span<GLXFBConfig>(*fbconfigs, fbcount)) {
		int getid {};
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_FBCONFIG_ID, &getid);
		if(getid == static_cast<int>(glConfigNumber(id))) {
			ret = config;
			break;
		}
	}

	::XFree(fbconfigs);
	return ret;
}

unsigned int GlxSetup::visualID(GlConfigID id) const
{
	auto glxfbc = glxConfig(id);
	if(!glxfbc) {
		warning("ny::GlxSetup::visualID: invalid gl config id");
		return 0u;
	}

	int visualid;
	::glXGetFBConfigAttrib(xDisplay_, glxfbc, GLX_VISUAL_ID, &visualid);

	return visualid;
}

// GlxSurface
GlxSurface::GlxSurface(Display& xdpy, unsigned int xDrawable, const GlConfig& config)
	: xDisplay_(&xdpy), xDrawable_(xDrawable), config_(config)
{
}

bool GlxSurface::apply(std::error_code& ec) const
{
	ec.clear();
	::glXSwapBuffers(xDisplay_, xDrawable_);
	return true;
}

// GlxContext
GlxContext::GlxContext(const GlxSetup& setup, GLXContext context, const GlConfig& config)
: setup_(&setup), glxContext_(context)
{
	GlContext::initContext(GlApi::gl, config, nullptr);
}

GlxContext::GlxContext(const GlxSetup& setup, const GlContextSettings& settings)
	: setup_(&setup)
{
	// test for config errors
	auto major = settings.version.major;
	auto minor = settings.version.minor;
	auto api = settings.version.api;

	if(settings.forceVersion && !ext::createContextAttribsARB) {
		constexpr auto msg = "ny::GlxContext: no createContextAttribsARB, cannot force version";
		throw GlContextError(GlContextErrc::invalidVersion, msg);
	}

	if(api == GlApi::gl && !ext::hasProfileES) {
		if(!settings.forceVersion) api = GlApi::gl;
		else throw GlContextError(GlContextErrc::invalidApi, "ny::GlxContext: ES no supported");
	}

	// config
	GlConfig glConfig;
	GLXFBConfig glxConfig;

	if(settings.config) {
		glConfig = setup.config(settings.config);
		glxConfig = setup.glxConfig(settings.config);
	} else {
		glConfig = setup.defaultConfig();
		glxConfig = setup.glxConfig(glConfig.id);
	}

	if(!glxConfig)
		throw GlContextError(GlContextErrc::invalidConfig, "ny::GlxContext");

	// shared
	GLXContext glxShareContext = nullptr;
	if(settings.share) {
		auto shareCtx = dynamic_cast<GlxContext*>(settings.share);
		if(!shareCtx)
			throw GlContextError(GlContextErrc::invalidSharedContext, "ny::GlxContext");

		glxShareContext = shareCtx->glxContext();
	}

	// test for extensions

	// set a new error handler
	auto oldErrorHandler = ::XSetErrorHandler(&ctxErrorHandler);
	if(ext::createContextAttribsARB) {
		std::vector<int> attributes;
		std::vector<std::pair<unsigned int, unsigned int>> versionPairs;

		// switch default versions and checked version pairs
		// also perform version sanity checks
		if(api == GlApi::gles) {
			if(major == 0 && minor == 0) {
				major = 3;
				minor = 2;
			}

			versionPairs = {{3, 2}, {3, 1}, {3, 0}, {2, 0}, {1, 1}, {1, 0}};
			if(major < 1 || major > 3 || minor > 2)
				throw GlContextError(GlContextErrc::invalidVersion, "ny::GlxContext");

		} else {
			if(major == 0 && minor == 0) {
				major = 4;
				minor = 5;
			}

			versionPairs = {{4, 5}, {3, 3}, {3, 2}, {3, 1}, {3, 0}, {1, 2}, {1, 0}};
			if(major < 1 || major > 4 || minor > 5)
				throw GlContextError(GlContextErrc::invalidVersion, "ny::GlxContext");
		}

		// profile
		if(ext::hasProfile) {
			attributes.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);

			if(ext::hasProfileES && api == GlApi::gles)
				attributes.push_back(GLX_CONTEXT_ES2_PROFILE_BIT_EXT);
			else if(settings.compatibility)
				attributes.push_back(GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);
			else
				attributes.push_back(GLX_CONTEXT_CORE_PROFILE_BIT_ARB);
		}

		// forward compatible, debug
		auto flags = 0u;
		if(settings.forwardCompatible) flags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
		if(settings.debug) flags |= GLX_CONTEXT_DEBUG_BIT_ARB;

		attributes.push_back(GLX_CONTEXT_FLAGS_ARB);
		attributes.push_back(flags);

		// version
		attributes.push_back(GLX_CONTEXT_MAJOR_VERSION_ARB);
		attributes.push_back(major);
		attributes.push_back(GLX_CONTEXT_MINOR_VERSION_ARB);
		attributes.push_back(minor);

		// null-terminated
		attributes.push_back(0);

		glxContext_ = ext::createContextAttribsARB(xDisplay(), glxConfig, glxShareContext,
			true, attributes.data());

		if(!glxContext_ && !settings.forceVersion) {
			for(const auto& p : versionPairs) {
				attributes[attributes.size() - 4] = p.first;
				attributes[attributes.size() - 2] = p.second;
				glxContext_ = ext::createContextAttribsARB(xDisplay(), glxConfig, glxShareContext,
					true, attributes.data());

				if(glxContext_) break;
			}

			if(!glxContext_) {
				warning("ny::GlxContext: could not create context for any tried version");
				if(lastErrorCode == glxBadFBConfig)
					warning("ny::GlxContext: context creation failed with glxbadfbconfig");
			}
		} else if(!glxContext_) {
			warning("ny::GlxContext: could not create context for forced version");
		}
	}


	if(!glxContext_ && !settings.forceVersion) {
		warning("ny::GlxContext: failed to create modern context, trying legacy method");
		glxContext_ = ::glXCreateNewContext(xDisplay(), glxConfig, GLX_RGBA_TYPE,
			glxShareContext, true);
	}

	if(!glxContext_ && !settings.forceVersion) {
		glxContext_ = ::glXCreateNewContext(xDisplay(), glxConfig, GLX_RGBA_TYPE,
			glxShareContext, false);
	}

	::XSync(xDisplay(), false);
	::XSetErrorHandler(oldErrorHandler);

	if(!glxContext_)
		throw std::runtime_error("ny::GlxContext: failed to create glx context in any way.");

	if(!::glXIsDirect(xDisplay(), glxContext_))
		warning("ny::GlxContext: could only create indirect gl context");

	GlContext::initContext(GlApi::gl, glConfig, settings.share);
}

GlxContext::~GlxContext()
{
	if(glxContext_) {
		std::error_code ec;
		if(!makeNotCurrent(ec))
			warning("ny::~GlxContext: failed to make the context not current: ", ec.message());

		::glXDestroyContext(xDisplay(), glxContext_);
	}
}

bool GlxContext::compatible(const GlSurface& surface) const
{
	if(!GlContext::compatible(surface)) return false;

	auto glxSurface = dynamic_cast<const GlxSurface*>(&surface);
	return (glxSurface);
}

GlContextExtensions GlxContext::contextExtensions() const
{
	GlContextExtensions ret;
	if(ext::swapIntervalEXT) ret |= GlContextExtension::swapControl;
	if(ext::hasSwapControlTear) ret |= GlContextExtension::swapControlTear;
	return ret;
}

bool GlxContext::swapInterval(int interval, std::error_code& ec) const
{
	ec.clear();

	if(!ext::swapIntervalEXT) {
		ec = {GlContextErrc::extensionNotSupported};
		return false;
	}

	if(!ext::hasSwapControlTear && interval < 0) {
		warning("ny::GlxContext::swapInterval: negative interval, no swap_control_tear");
		ec = {GlContextErrc::extensionNotSupported};
		return false;
	}

	const GlSurface* currentSurface;
	GlContext::current(&currentSurface);

	auto currentGlxSurface = dynamic_cast<const GlxSurface*>(currentSurface);
	if(!currentGlxSurface) {
		// ec = {GlContextErrorCode::surfaceNotCurrent}; // TODO: add error for this case
		return false;
	}

	ext::swapIntervalEXT(xDisplay(), currentGlxSurface->xDrawable(), interval);

	// TODO: handle possible error into ec
	return true;
}

bool GlxContext::makeCurrentImpl(const GlSurface& surface, std::error_code& ec)
{
	ec.clear();

	auto drawable = dynamic_cast<const GlxSurface*>(&surface)->xDrawable();
	if(!::glXMakeCurrent(xDisplay(), drawable, glxContext_)) {
		// TODO: handle error into ec
		warning("ny::GlxContext::makeCurrentImpl (glXMakeCurrent) failed");
		return false;
	}

	return true;
}

bool GlxContext::makeNotCurrentImpl(std::error_code& ec)
{
	ec.clear();

	if(!::glXMakeCurrent(xDisplay(), 0, nullptr)) {
		// TODO: handle error into ec
		warning("ny::GlxContext::makeNotCurrentImpl (glXMakeCurrent) failed");
		return false;
	}

	return true;
}

// GlxWindowContext
GlxWindowContext::GlxWindowContext(X11AppContext& ac, const GlxSetup& setup,
	const X11WindowSettings& settings)
{
	appContext_ = &ac;

	auto configid = settings.gl.config;
	if(!configid) {
		if(settings.transparent) configid = setup.defaultTransparentConfig().id;
		if(!configid) {
			if(settings.transparent) warning("ny::GlxWindowContext: no transparent config");
			configid = setup.defaultConfig().id;
		}
	}

	auto config = setup.config(configid);
	visualID_ = setup.visualID(configid);
	depth_ = x11::visualDepth(ac.xDefaultScreen(), visualID_);

	X11WindowContext::create(ac, settings);

	surface_ = std::make_unique<GlxSurface>(appContext().xDisplay(), xWindow(), config);
	if(settings.gl.storeSurface) *settings.gl.storeSurface = surface_.get();
}

Surface GlxWindowContext::surface()
{
	return {*surface_};
}

} // namespace ny
