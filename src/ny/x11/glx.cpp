// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/glx.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/x11/glxApi.hpp>
#include <ny/surface.hpp>
#include <ny/log.hpp>

#include <nytl/span.hpp>

#include <xcb/xcb.h>

#include <algorithm>
#include <mutex>

// TODO: this needs massive error (code) handling improvements (see GlContext implementation)
// NOTE: For glx GlConfig objects, the id member is the GLX_FB_CONFIG_ID of the associated
// glx fb config.

namespace ny {
namespace {

bool loadExtensions(Display& dpy)
{
	static std::once_flag cof {};
	static bool valid {};

	std::call_once(cof, [&]{
		// we call both since they are different on some platforms
		auto client = ::glXGetClientString(&dpy, GLX_EXTENSIONS);
		auto exts = ::glXQueryExtensionsString(&dpy, 0);

		if(!client || !exts) {
			ny_warn("::glx::loadExtensions:"_src, "failed to retrieve extension string");
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

} // namespace glx

// GlxSetup
GlxSetup::GlxSetup(const X11AppContext& ac, unsigned int screenNum) : appContext_(&ac)
{
	// query version
	// we need at least glx 1.3 for fb configs
	// should be supported on almost all machines
	int major, minor;
	::glXQueryVersion(&xDisplay(), &major, &minor);
	if(major < 1 || (major == 1 && minor < 3))
		throw std::runtime_error("ny::GlxSetup: glx 1.3 not supported");

	if(!loadExtensions(xDisplay()))
		throw std::runtime_error("ny::GlxSetup: failed to load extensions");

	int fbcount = 0;
	GLXFBConfig* fbconfigs = ::glXGetFBConfigs(&xDisplay(), screenNum, &fbcount);
	if(!fbconfigs || !fbcount)
		throw std::runtime_error("ny::GlxSetup: could not retrieve any fb configs");

	configs_.reserve(fbcount);
	auto highestRating = 0u;
	auto highestTransparentRating = 0u;

	for(auto& config : nytl::Span<GLXFBConfig>(*fbconfigs, fbcount)) {
		// NOTE: we don't query/handle GLX_TRANSPARENT_TYPE since it is usually
		// note correctly set

		GlConfig glconf;
		int r, g, b, a, id, depth, stencil, doubleBuffer, visual;

		::glXGetFBConfigAttrib(&xDisplay(), config, GLX_FBCONFIG_ID, &id);
		::glXGetFBConfigAttrib(&xDisplay(), config, GLX_STENCIL_SIZE, &stencil);
		::glXGetFBConfigAttrib(&xDisplay(), config, GLX_DEPTH_SIZE, &depth);
		::glXGetFBConfigAttrib(&xDisplay(), config, GLX_DOUBLEBUFFER, &doubleBuffer);
		::glXGetFBConfigAttrib(&xDisplay(), config, GLX_VISUAL_ID, &visual);

		::glXGetFBConfigAttrib(&xDisplay(), config, GLX_RED_SIZE, &r);
		::glXGetFBConfigAttrib(&xDisplay(), config, GLX_GREEN_SIZE, &g);
		::glXGetFBConfigAttrib(&xDisplay(), config, GLX_BLUE_SIZE, &b);
		::glXGetFBConfigAttrib(&xDisplay(), config, GLX_ALPHA_SIZE, &a);

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
	appContext_ = other.appContext_;
	defaultConfig_ = other.defaultConfig_;
	defaultTransparentConfig_ = other.defaultTransparentConfig_;
	configs_ = std::move(other.configs_);

	other.appContext_ = {};
	other.defaultConfig_ = {};
}

GlxSetup& GlxSetup::operator=(GlxSetup&& other) noexcept
{
	appContext_ = other.appContext_;
	defaultConfig_ = other.defaultConfig_;
	defaultTransparentConfig_ = other.defaultTransparentConfig_;
	configs_ = std::move(other.configs_);

	other.appContext_ = {};
	other.defaultConfig_ = {};

	return *this;
}

std::unique_ptr<GlContext> GlxSetup::createContext(const GlContextSettings& settings) const
{
	return std::make_unique<GlxContext>(*this, settings);
}

void* GlxSetup::procAddr(std::string_view name) const
{
	auto data = reinterpret_cast<const unsigned char*>(name.data());
	auto ret = reinterpret_cast<void*>(::glXGetProcAddress(data));
	return ret;
}

GLXFBConfig GlxSetup::glxConfig(GlConfigID id) const
{
	int fbcount {};
	auto* fbconfigs = ::glXGetFBConfigs(&xDisplay(), 0, &fbcount);
	if(!fbconfigs || !fbcount) return nullptr;

	GLXFBConfig ret {};
	for(auto& config : nytl::Span<GLXFBConfig>(*fbconfigs, fbcount)) {
		int getid {};
		::glXGetFBConfigAttrib(&xDisplay(), config, GLX_FBCONFIG_ID, &getid);
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
		ny_warn("::GlxSetup::visualID"_src, "invalid gl config id");
		return 0u;
	}

	int visualid;
	::glXGetFBConfigAttrib(&xDisplay(), glxfbc, GLX_VISUAL_ID, &visualid);

	return visualid;
}

Display& GlxSetup::xDisplay() const
{
	return appContext().xDisplay();
}

// GlxSurface
GlxSurface::GlxSurface(const GlxSetup& setup, unsigned int xDrawable, const GlConfig& config)
	: setup_(setup), xDrawable_(xDrawable), config_(config)
{
}

GlxSurface::~GlxSurface()
{
	GlContext* context;
	if(isCurrent(&context)) {
		std::error_code ec;
		if(!context->makeNotCurrent(ec))
			ny_warn("~GlxSurface"_scope, "failed not make not current: {}", ec.message());
	}

	if(isCurrentInAnyThread())
		ny_error("~GlxSurface:"_scope, "still current in another thread");
}

bool GlxSurface::apply(std::error_code& ec) const
{
	ec.clear();
	auto& errorCat = appContext().errorCategory();
	errorCat.resetLastXlibError();

	::glXSwapBuffers(&xDisplay(), xDrawable_);
	if(errorCat.lastXlibError()) {
		ec = errorCat.lastXlibError();
		return false;
	}

	return true;
}

// GlxContext
GlxContext::GlxContext(const GlxSetup& setup, GLXContext context, const GlConfig& config)
: setup_(setup), glxContext_(context)
{
	GlContext::initContext(GlApi::gl, config, nullptr);
}

GlxContext::GlxContext(const GlxSetup& setup, const GlContextSettings& settings)
	: setup_(setup)
{
	dlg_source("GlxContext()"_scope);

	// test config
	auto api = settings.api;
	if(api == GlApi::gles && !hasProfileES) {
		constexpr auto msg = "ny::GlxContext: no profile_es2 ext, cannot create gles context";
		throw GlContextError(GlContextErrc::invalidApi, msg);
	}

	if(api == GlApi::none)
		api = GlApi::gl;

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

	// set a new error handler
	auto& errorCat = appContext().errorCategory();

	if(createContextAttribsARB) {
		std::vector<std::pair<unsigned int, unsigned int>> versionPairs;
		std::vector<int> attributes;
		attributes.reserve(16);

		// switch default versions and checked version pairs
		if(api == GlApi::gles) versionPairs = {{3, 2}, {3, 1}, {3, 0}, {2, 0}, {1, 1}, {1, 0}};
		else versionPairs = {{4, 5}, {3, 3}, {3, 2}, {3, 1}, {3, 0}, {1, 2}, {1, 0}};

		// profile
		if(hasProfile) {
			attributes.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);

			if(hasProfileES && api == GlApi::gles)
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
		// will be set later, during the testing loop
		attributes.push_back(GLX_CONTEXT_MAJOR_VERSION_ARB);
		attributes.push_back(0);
		attributes.push_back(GLX_CONTEXT_MINOR_VERSION_ARB);
		attributes.push_back(0);

		// null-terminated
		attributes.push_back(0);

		for(const auto& p : versionPairs) {
			attributes[attributes.size() - 4] = p.first;
			attributes[attributes.size() - 2] = p.second;

			errorCat.resetLastXlibError();
			glxContext_ = createContextAttribsARB(&xDisplay(), glxConfig, glxShareContext,
				true, attributes.data());

			auto errorCode = errorCat.lastXlibError();
			if(errorCode && errorCode.message() != "GLXBadFBConfig")
				ny_warn("unexpected error: {}", errorCode.message());

			if(glxContext_)
				break;
		}
	}

	// try legacy version
	// direct
	if(!glxContext_) {
		errorCat.resetLastXlibError();
		ny_info("could not create modern context, trying legacy version");
		glxContext_ = ::glXCreateNewContext(&xDisplay(), glxConfig, GLX_RGBA_TYPE,
			glxShareContext, true);

		if(!glxContext_)
			ny_warn("glxCreateNewContext: {}", errorCat.lastXlibError().message());
	}

	// indirect
	if(!glxContext_) {
		errorCat.resetLastXlibError();
		glxContext_ = ::glXCreateNewContext(&xDisplay(), glxConfig, GLX_RGBA_TYPE,
			glxShareContext, false);

		if(!glxContext_)
			ny_warn("glxCreateNewContext failed: {}", errorCat.lastXlibError().message());
	}

	::XSync(&xDisplay(), false);

	if(!glxContext_)
		throw std::runtime_error("ny::GlxContext: failed to create glx context in any way.");

	if(!::glXIsDirect(&xDisplay(), glxContext_))
		ny_info("could only create indirect gl context");

	GlContext::initContext(GlApi::gl, glConfig, settings.share);
}

GlxContext::~GlxContext()
{
	if(glxContext_) {
		std::error_code ec;
		if(!makeNotCurrent(ec))
			ny_warn("~GlxContext"_scope, "failed to make context not current: {}", ec.message());

		if(isCurrentInAnyThread())
			ny_error("~GlxContext"_scope, "still current in a thread. Can't do much about it");

		::glXDestroyContext(&xDisplay(), glxContext_);
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
	if(swapIntervalEXT) ret |= GlContextExtension::swapControl;
	if(hasSwapControlTear) ret |= GlContextExtension::swapControlTear;
	return ret;
}

bool GlxContext::swapInterval(int interval, std::error_code& ec) const
{
	ec.clear();

	if(!swapIntervalEXT) {
		ec = {GlContextErrc::extensionNotSupported};
		return false;
	}

	if(!hasSwapControlTear && interval < 0) {
		ny_warn("::glx::swapInterval"_src, "negative interval, no swap_control_tear");
		ec = {GlContextErrc::extensionNotSupported};
		return false;
	}

	const GlSurface* currentSurface;
	GlContext::current(&currentSurface);

	auto currentGlxSurface = dynamic_cast<const GlxSurface*>(currentSurface);
	if(!isCurrent() || !currentGlxSurface) {
		ec = {GlContextErrc::contextNotCurrent};
		return false;
	}

	auto& errorCat = appContext().errorCategory();
	errorCat.resetLastXlibError();

	swapIntervalEXT(&xDisplay(), currentGlxSurface->xDrawable(), interval);
	if(errorCat.lastXlibError()) {
		ec = errorCat.lastXlibError();
		return false;
	}

	return true;
}

bool GlxContext::makeCurrentImpl(const GlSurface& surface, std::error_code& ec)
{
	ec.clear();
	auto& errorCat = appContext().errorCategory();
	errorCat.resetLastXlibError();

	auto drawable = dynamic_cast<const GlxSurface*>(&surface)->xDrawable();
	if(!::glXMakeCurrent(&xDisplay(), drawable, glxContext_)) {
		ec = errorCat.lastXlibError();
		return false;
	}

	return true;
}

bool GlxContext::makeNotCurrentImpl(std::error_code& ec)
{
	ec.clear();
	auto& errorCat = appContext().errorCategory();
	errorCat.resetLastXlibError();

	if(!::glXMakeCurrent(&xDisplay(), 0, nullptr)) {
		ec = errorCat.lastXlibError();
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
			if(settings.transparent)
				ny_warn("::glx::WindowContext"_src, "no transparent config");
			configid = setup.defaultConfig().id;
		}
	}

	auto config = setup.config(configid);
	visualID_ = setup.visualID(configid);
	depth_ = x11::visualDepth(ac.xDefaultScreen(), visualID_);

	X11WindowContext::create(ac, settings);
	auto glxSetup = appContext().glxSetup();
	if(!glxSetup)
		throw std::runtime_error("ny::GlxWindowContext: failed to init glx");

	surface_ = std::make_unique<GlxSurface>(*glxSetup, xWindow(), config);
	if(settings.gl.storeSurface) *settings.gl.storeSurface = surface_.get();
}

Surface GlxWindowContext::surface()
{
	return {*surface_};
}

} // namespace ny
