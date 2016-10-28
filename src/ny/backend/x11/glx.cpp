#include <ny/backend/x11/glx.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/glxApi.hpp>
#include <ny/backend/integration/surface.hpp>
#include <ny/base/log.hpp>

#include <nytl/misc.hpp>
#include <nytl/range.hpp>

// #include <dlfcn.h>
#include <algorithm>

namespace ny
{

namespace
{
	//This error handler might be signaled when glx context creation fails
	bool errorOccured = false;
	int ctxErrorHandler(Display*, XErrorEvent*)
	{
		warning("GlxContext::GlxContext: Error occured");
	    errorOccured = true;
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
}

GlxContextWrapper::GlxContextWrapper(Display* dpy, GLXFBConfig fbc) : xDisplay(dpy)
{
	assureGlxLoaded(dpy);

    int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);
    using Pfn = GLXContext(*)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

    if(GLAD_GLX_ARB_create_context && glXCreateContextAttribsARB)
    {
        int attribs[] =
        {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
            GLX_CONTEXT_MINOR_VERSION_ARB, 5,
			GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };

		unsigned int versionPairs[][2] = {{3, 3}, {3, 2}, {3, 1}, {3, 0}, {1, 2}, {1, 0}};
		for(const auto& p : versionPairs)
		{
			context = glXCreateContextAttribsARB(xDisplay, fbc, nullptr, true, attribs);
			XSync(xDisplay, False);
			if(!context || errorOccured)
			{
				errorOccured = false;
				context = nullptr;
				attribs[1] = p[0];
				attribs[3] = p[1];
				continue;
			}

			break;
		}
    }

    if(!context) context = glXCreateNewContext(xDisplay, fbc, GLX_RGBA_TYPE, 0, True);

    XSetErrorHandler(oldHandler);
    if(!context || errorOccured)
    {
		errorOccured = true;
        throw std::runtime_error("ny::Glx: failed to create glx Context.");
    }
}

GlxContextWrapper::~GlxContextWrapper()
{
    if(context) glXDestroyContext(xDisplay, context);
}

GlxContextWrapper::GlxContextWrapper(GlxContextWrapper&& other) noexcept
	: xDisplay(other.xDisplay), context(other.context)
{
	other.context = {};
}

GlxContextWrapper& GlxContextWrapper::operator=(GlxContextWrapper&& other) noexcept
{
    if(context) glXDestroyContext(xDisplay, context);

	xDisplay = other.xDisplay;
	context = other.context;
	other.context = {};

	return *this;
}

//GlxContext
GlxContext::GlxContext(Display* dpy, unsigned int drawable, GLXContext ctx, GLXFBConfig fbc)
	: xDisplay_(dpy), drawable_(drawable), glxContext_(ctx)
{
	// glxWindow_ = glXCreateWindow(xDisplay, fbc, wc_->xWindow(), 0);

	int depth = 0, stencil = 0;
	if(fbc)
	{
		glXGetFBConfigAttrib(xDisplay_, fbc, GLX_STENCIL_SIZE, &stencil);
		glXGetFBConfigAttrib(xDisplay_, fbc, GLX_DEPTH_SIZE, &depth);
	}

	GlContext::initContext(GlApi::gl, depth, stencil);
}

GlxContext::~GlxContext()
{
	makeNotCurrent();
}

bool GlxContext::makeCurrentImpl()
{
    if(!glXMakeCurrent(xDisplay_, drawable_, glxContext_))
    {
		warning("ny::GlxContext::makeCurrentImpl: glxmakecurrent failed");
        return false;
    }

    return true;
}

bool GlxContext::makeNotCurrentImpl()
{
    if(!glXMakeCurrent(xDisplay_, 0, nullptr))
    {
		warning("ny::GlxContext::makeNotCurrentImpl: glxmakecurrent failed");
        return false;

    }

    return true;
}

bool GlxContext::apply()
{
	GlContext::apply();
    glXSwapBuffers(xDisplay_, drawable_);
    return true;
}

void* GlxContext::procAddr(const char* name) const
{
	auto ret = reinterpret_cast<void*>(::glXGetProcAddress(name));
	// if(!ret) ret = reinterpret_cast<void*>(::dlsym(glLibHandle(), name));
	return ret;
}


//GlxWindowContext
GlxWindowContext::GlxWindowContext(X11AppContext& ac, const X11WindowSettings& settings)
{
	appContext_ = &ac;

	GLXFBConfig fbc;
	initFbcVisual(fbc);

	X11WindowContext::create(ac, settings);

	auto ctx = ac.glxContext(fbc);
	glxContext_ = std::make_unique<GlxContext>(appContext().xDisplay(), xWindow(), ctx, fbc);

	if(settings.gl.storeContext) *settings.gl.storeContext = glxContext_.get();
}

void GlxWindowContext::initFbcVisual(GLXFBConfig& fbconfig)
{

	auto* const xDisplay = appContext().xDisplay();
	const auto screenNumber = appContext().xDefaultScreenNumber();

	assureGlxLoaded(xDisplay);
    constexpr int attribs[] =
    {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        None
    };

	//TODO; better choosing...
	//maybe there is no fbconfig with alpha, then just one without
	//compare the returned ones, dont just choose the first one
    int fbcount = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(xDisplay, screenNumber, attribs, &fbcount);
    if (!fbc || !fbcount) throw std::runtime_error("ny::GlxWC: failed to choose fbconfig");

	fbconfig = fbc[0];
	XFree(fbc);

	int visualID;
	glXGetFBConfigAttrib(xDisplay, fbconfig, GLX_VISUAL_ID, &visualID);
	if(!visualID) throw std::runtime_error("ny::GlxWC: failed to retrieve glx_visual_id");

	visualID_ = visualID;
}

bool GlxWindowContext::surface(Surface& surface)
{
	surface.type = Surface::Type::gl;
	surface.gl = glxContext_.get();
	return true;
}

}
