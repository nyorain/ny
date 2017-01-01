// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/glx.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/glxApi.hpp>
#include <ny/surface.hpp>
#include <ny/log.hpp>

#include <nytl/span.hpp>

#include <xcb/xcb.h>
#include <algorithm>

//For glx GlConfig objects, the id member is the GLX_FB_CONFIG_ID of the associated
//glx fb config.

namespace ny
{

namespace
{
	//This error handler might be signaled when glx context creation fails
	int ctxErrorHandler(Display* display, XErrorEvent* event)
	{
		char buffer[256];
		::XGetErrorText(display, event->error_code, buffer, 255);

		log("ny::GlxContext: Error occured during context creation: ",
			(int) event->error_code, ", ", buffer);

	    return 0;
	}

	void assureGlxLoaded(Display* dpy)
	{
		static bool loaded = false;
		if(!loaded)
		{
			gladLoadGLX(dpy, DefaultScreen(dpy));
			loaded = true;
		}
	}

	unsigned int findVisualDepth(const X11AppContext& ac, unsigned int visualID)
	{
		auto depthi = xcb_screen_allowed_depths_iterator(&ac.xDefaultScreen());
		for(; depthi.rem; xcb_depth_next(&depthi))
		{
			auto visuali = xcb_depth_visuals_iterator(depthi.data);
			for(; visuali.rem; xcb_visualtype_next(&visuali))
				if(visuali.data->visual_id == visualID) return depthi.data->depth;
		}

		return 0u;
	}
}

//GlxSetup
GlxSetup::GlxSetup(const X11AppContext& ac, unsigned int screenNum) : xDisplay_(&ac.xDisplay())
{
	assureGlxLoaded(xDisplay_);
    constexpr int attribs[] =
    {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        0
    };

    int fbcount = 0;
    GLXFBConfig* fbconfigs = ::glXChooseFBConfig(xDisplay_, screenNum, attribs, &fbcount);
	if(!fbconfigs || !fbcount)
		throw std::runtime_error("ny::GlxSetup: could not retrieve any fb configs");

	configs_.reserve(fbcount);
	auto highestRating = 0u;
	for(auto& config : nytl::Span<GLXFBConfig>(*fbconfigs, fbcount))
	{
		GlConfig glconf;
		int r, g, b, a, id, depth, stencil, doubleBuffer;

		::glXGetFBConfigAttrib(xDisplay_, config, GLX_FBCONFIG_ID, &id);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_STENCIL_SIZE, &stencil);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_DEPTH_SIZE, &depth);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_DOUBLEBUFFER, &doubleBuffer);

		::glXGetFBConfigAttrib(xDisplay_, config, GLX_RED_SIZE, &r);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_GREEN_SIZE, &g);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_BLUE_SIZE, &b);
		::glXGetFBConfigAttrib(xDisplay_, config, GLX_ALPHA_SIZE, &a);

		glconf.depth = depth;
		glconf.stencil = stencil;
		glconf.red = r;
		glconf.green = g;
		glconf.blue = b;
		glconf.alpha = a;
		glconf.id = glConfigID(id);
		glconf.doublebuffer = doubleBuffer;

		configs_.push_back(glconf);
		auto xvDepth = findVisualDepth(ac, id);

		auto rating = rate(configs_.back());

		if(xvDepth == 24) rating *= 2;
		else if(xvDepth == 32) rating *= 3;
		if(rating > highestRating)
		{
			highestRating = rating;
			defaultConfig_ = &configs_.back(); //configs_ will never be reallocated
		}
	}

	::XFree(fbconfigs);
}

GlxSetup::GlxSetup(GlxSetup&& other) noexcept
{
	xDisplay_ = other.xDisplay_;
	defaultConfig_ = other.defaultConfig_;

	configs_ = std::move(other.configs_);
	glLibrary_ = std::move(other.glLibrary_);

	other.xDisplay_ = {};
	other.defaultConfig_ = {};
}

GlxSetup& GlxSetup::operator=(GlxSetup&& other) noexcept
{
	xDisplay_ = other.xDisplay_;
	defaultConfig_ = other.defaultConfig_;

	configs_ = std::move(other.configs_);
	glLibrary_ = std::move(other.glLibrary_);

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
	auto ret = reinterpret_cast<void*>(::glXGetProcAddress(name));
	return ret;
}

GLXFBConfig GlxSetup::glxConfig(GlConfigID id) const
{
	int configCount;
	int configAttribs[] = {GLX_FBCONFIG_ID, static_cast<int>(glConfigNumber(id)), 0};

	//TODO
	auto screenNumber = 0;

	auto configs = ::glXChooseFBConfig(xDisplay_, screenNumber, configAttribs, &configCount);
	if(!configs) return nullptr;

	auto ret = *configs;
	::XFree(configs);
	return ret;
}

unsigned int GlxSetup::visualID(GlConfigID id) const
{
	auto glxfbc = glxConfig(id);
	if(glxfbc) return 0u;

	int visualid;
	::glXGetFBConfigAttrib(xDisplay_, glxfbc, GLX_VISUAL_ID, &visualid);

	return visualid;
}

//GlxSurface
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

//GlxContext
GlxContext::GlxContext(const GlxSetup& setup, GLXContext context, const GlConfig& config)
 : setup_(&setup), glxContext_(context)
{
	GlContext::initContext(GlApi::gl, config, nullptr);
}

GlxContext::GlxContext(const GlxSetup& setup, const GlContextSettings& settings)
	: setup_(&setup)
{
	auto major = settings.version.major;
	auto minor = settings.version.minor;

	if(major == 0 && minor == 0)
	{
		major = 4;
		minor = 5;
	}

	//test for logical errors
	if(settings.version.api != GlApi::gl)
		throw GlContextError(GlContextErrc::invalidApi, "ny::WglContext");

	if(major < 1 || major > 4 || minor > 5)
		throw GlContextError(GlContextErrc::invalidVersion, "ny::GlxContext");

	//config
	GlConfig glConfig;
	GLXFBConfig glxConfig;

	if(settings.config)
	{
		glConfig = setup.config(settings.config);
		glxConfig = setup.glxConfig(settings.config);
	}
	else
	{
		glConfig = setup.defaultConfig();
		glxConfig = setup.glxConfig(glConfig.id);
	}

	if(!glxConfig) throw GlContextError(GlContextErrc::invalidConfig, "ny::GlxContext");

	//shared
	GLXContext glxShareContext = nullptr;
	if(settings.share)
	{
		auto shareCtx = dynamic_cast<GlxContext*>(settings.share);
		if(!shareCtx)
			throw GlContextError(GlContextErrc::invalidSharedContext, "ny::EglContext");

		glxShareContext = shareCtx->glxContext();
	}

	//set a new error handler
    auto oldErrorHandler = ::XSetErrorHandler(&ctxErrorHandler);

    if(GLAD_GLX_ARB_create_context && glXCreateContextAttribsARB)
    {
		std::vector<int> contextAttribs;

		//profile
		contextAttribs.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);
		if(!settings.compatibility) contextAttribs.push_back(GLX_CONTEXT_CORE_PROFILE_BIT_ARB);
		else contextAttribs.push_back(GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);

		//forward compatible, debug
		auto flags = 0u;
		if(settings.forwardCompatible) flags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
		if(settings.debug) flags |= GLX_CONTEXT_DEBUG_BIT_ARB;

		contextAttribs.push_back(GLX_CONTEXT_FLAGS_ARB);
		contextAttribs.push_back(flags);

		//version
		contextAttribs.push_back(GLX_CONTEXT_MAJOR_VERSION_ARB);
		contextAttribs.push_back(major);
		contextAttribs.push_back(GLX_CONTEXT_MINOR_VERSION_ARB);
		contextAttribs.push_back(minor);

		//end
		contextAttribs.push_back(0);

		glxContext_ = ::glXCreateContextAttribsARB(xDisplay(), glxConfig, glxShareContext,
			true, contextAttribs.data());

		if(!glxContext_ && !settings.forceVersion)
		{
			//those versions will be tried to create when the version specified in
			//the passed settings fails and the passed version should not be forced.
			constexpr std::pair<unsigned int, unsigned int> versionPairs[] =
				{{4, 5}, {3, 3}, {3, 2}, {3, 1}, {3, 0}, {1, 2}, {1, 0}};

			for(const auto& p : versionPairs)
			{
				contextAttribs[contextAttribs.size() - 4] = p.first;
				contextAttribs[contextAttribs.size() - 2] = p.second;
				glxContext_ = glXCreateContextAttribsARB(xDisplay(), glxConfig, glxShareContext,
					true, contextAttribs.data());

				if(glxContext_) break;
			}
		}
    }


    if(!glxContext_ && !settings.forceVersion)
	{
		warning("ny::GlxContext: failed to create modern context, trying legacy method");
		glxContext_ = ::glXCreateNewContext(xDisplay(), glxConfig, GLX_RGBA_TYPE,
			glxShareContext, true);
	}

    if(!glxContext_ && !settings.forceVersion)
	{
		glxContext_ = ::glXCreateNewContext(xDisplay(), glxConfig, GLX_RGBA_TYPE,
			glxShareContext, false);
	}

	::XSync(xDisplay(), false);
    ::XSetErrorHandler(oldErrorHandler);

	if(!glxContext_)
		throw std::runtime_error("ny::GlxContext: failed to create glx Context in any way.");

	if(!::glXIsDirect(xDisplay(), glxContext_))
		warning("ny::GlxContext: could only create indirect gl context -> worse performance");

	GlContext::initContext(GlApi::gl, glConfig, settings.share);
}

GlxContext::~GlxContext()
{
	if(glxContext_)
	{
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
	if(GLAD_GLX_EXT_swap_control) ret |= GlContextExtension::swapControl;
	// if(GLAD_GLX_EXT_swap_control_tear) ret |= GlContextExtension::swapControlTear;
	return ret;
}

bool GlxContext::swapInterval(int interval, std::error_code& ec) const
{
	//TODO: check for interval < 0 and tear extensions not supported.
	ec.clear();

	if(!GLAD_GLX_EXT_swap_control || !glXSwapIntervalEXT)
	{
		ec = {GlContextErrc::extensionNotSupported};
		return false;
	}

	const GlSurface* currentSurface;
	GlContext::current(&currentSurface);

	auto currentGlxSurface = dynamic_cast<const GlxSurface*>(currentSurface);
	if(!currentGlxSurface)
	{
		// ec = {GlContextErrorCode::surfaceNotCurrent}; //TODO: add error for this case
		return false;
	}

	::glXSwapIntervalEXT(xDisplay(), currentGlxSurface->xDrawable(), interval);

	//TODO: handle possible error into ec

	return true;
}

bool GlxContext::makeCurrentImpl(const GlSurface& surface, std::error_code& ec)
{
	ec.clear();

	auto drawable = dynamic_cast<const GlxSurface*>(&surface)->xDrawable();
    if(!::glXMakeCurrent(xDisplay(), drawable, glxContext_))
    {
		//TODO: handle error into ec
		warning("ny::GlxContext::makeCurrentImpl (glXMakeCurrent) failed");
        return false;
    }

    return true;
}

bool GlxContext::makeNotCurrentImpl(std::error_code& ec)
{
	ec.clear();

    if(!::glXMakeCurrent(xDisplay(), 0, nullptr))
    {
		//TODO: handle error into ec
		warning("ny::GlxContext::makeNotCurrentImpl (glXMakeCurrent) failed");
        return false;
    }

    return true;
}

//GlxWindowContext
GlxWindowContext::GlxWindowContext(X11AppContext& ac, const GlxSetup& setup,
	const X11WindowSettings& settings)
{
	appContext_ = &ac;

	auto configid = settings.gl.config;
	if(!configid) configid = setup.defaultConfig().id;

	auto config = setup.config(configid);
	visualID_ = setup.visualID(configid);
	depth_ = findVisualDepth(ac, visualID_);

	X11WindowContext::create(ac, settings);

	surface_ = std::make_unique<GlxSurface>(appContext().xDisplay(), xWindow(), config);
	if(settings.gl.storeSurface) *settings.gl.storeSurface = surface_.get();
}

Surface GlxWindowContext::surface()
{
	return {*surface_};
}

}
