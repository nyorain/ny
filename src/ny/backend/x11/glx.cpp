#include <ny/backend/x11/glx.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/x11/appContext.hpp>
#include <ny/draw/gl/drawContext.hpp>
#include <ny/base/log.hpp>

#include <nytl/misc.hpp>

#include <GL/glx.h>
#include <algorithm>

namespace ny
{

namespace
{
	//errorHandler
	bool errorOccured = 0;
	int ctxErrorHandler(Display*, XErrorEvent*)
	{
		sendWarning("GlxContext::GlxContext: Error occured");
	    errorOccured = true;
	    return 0;
	}
}

//
GlxContext::GlxContext(X11WindowContext& wc, GLXFBConfig fbc) : wc_(&wc)
{
	glxWindow_ = glXCreateWindow(xDisplay(), fbc, wc_->xWindow(), 0);

    int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);
    using procType = GLXContext(*)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

    procType glXCreateContextAttribsARB = 
		(procType) glXGetProcAddressARB((const GLubyte*) "glXCreateContextAttribsARB" );

    const char* glxExts = glXQueryExtensionsString(xDisplay(), DefaultScreen(xDisplay()));
	auto extVec = split(glxExts, ' ');
	
	//sendLog("glx extensions: ", glxExts);
	auto it = std::find(extVec.begin(), extVec.end(), "GLX_ARB_create_context");
	bool supported = (it != extVec.end());

    if(supported && glXCreateContextAttribsARB)
    {
        int attribs[] =
        {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 3,
            //GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            None
        };

        glxContext_ = glXCreateContextAttribsARB(xDisplay(), fbc, nullptr, True, attribs);
        XSync(xDisplay(), False);
        if(!glxContext_ || errorOccured)
        {
            errorOccured = 0;
            glxContext_ = nullptr;
			sendWarning("modern GL context could not be created, creating legacy GL context");
        }

        if(!glxContext_)
        {
            attribs[1] = 1;
            attribs[3] = 0;

            glxContext_ = glXCreateContextAttribsARB(xDisplay(), fbc, nullptr, True, attribs );
            XSync(xDisplay(), False);
            if(!glxContext_ || errorOccured)
            {
                errorOccured = 0;
                glxContext_ = nullptr;
				sendWarning("legacy GL context could not be created, trying old method");
            }
        }
    }

    if(!glxContext_)
    {
        glxContext_ = glXCreateNewContext(xDisplay(), fbc, GLX_RGBA_TYPE, 0, True);
    }


    XSetErrorHandler(oldHandler);
    if(!glxContext_ || errorOccured)
    {
        throw std::runtime_error("glxContext: could not create glx Context");
        return;
    }

    int depth, stencil = 0;
    glXGetFBConfigAttrib(xDisplay(), fbc, GLX_STENCIL_SIZE, &stencil);
    glXGetFBConfigAttrib(xDisplay(), fbc, GLX_DEPTH_SIZE, &depth);

	GlContext::initContext(Api::openGL, depth, stencil);
}

GlxContext::~GlxContext()
{
    if(glxContext_) glXDestroyContext(xDisplay(), glxContext_);
}

bool GlxContext::makeCurrentImpl()
{
    if(!glXMakeCurrent(xDisplay(), glxWindow_, glxContext_))
    {
		sendWarning("Glx::makeCurrentImpl: glxmakecurrent failed");
        return 0;
    }

    return 1;
}

bool GlxContext::makeNotCurrentImpl()
{
    if(!glXMakeCurrent(xDisplay(), 0, nullptr))
    {
		sendWarning("Glx::makeNotCurrentImpl: glxmakecurrent failed");
        return 0;

    }

    return 1;
}

bool GlxContext::apply()
{
	drawContext_.apply();

	GlContext::apply();
    glXSwapBuffers(xDisplay(), glxWindow_);
    return 1;
}

void GlxContext::size(const Vec2ui& size)
{
	makeCurrent();
	updateViewport(Rect2f({0.f, 0.f}, size));
}

/*
GLXFBConfig X11WindowContext::matchGLXVisualInfo()
{
#ifdef NY_WithGL	
    const int attribs[] =
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

    int glxMajor, glxMinor;
    if(!glXQueryVersion(xDisplay(), &glxMajor, &glxMinor) 
			|| ((glxMajor == 1) && (glxMinor < 3) ) || (glxMajor < 1))
    {
        throw std::runtime_error("Invalid glx version. glx Version must be > 1.3");
    }

    int fbcount = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(xDisplay(), DefaultScreen(xDisplay()), attribs, &fbcount);
    if (!fbc || !fbcount)
    {
        throw std::runtime_error("failed to retrieve fbconfig");
    }

    //get the config with the most samples
    int best_fbc = -1, worst_fbc = -1, best_num_samp = 0, worst_num_samp = 0;
    for(int i(0); i < fbcount; i++)
    {
        XVisualInfo *vi = glXGetVisualFromFBConfig(xDisplay(), fbc[i]);

        if(!vi) continue;

        int samp_buf, samples;
        glXGetFBConfigAttrib(xDisplay(), fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
        glXGetFBConfigAttrib(xDisplay(), fbc[i], GLX_SAMPLES, &samples);

        if(best_fbc < 0 || (samp_buf && samples > best_num_samp))
        {
            best_fbc = i;
            best_num_samp = samples;
        }

        if(worst_fbc < 0 || (!samp_buf || samples < worst_num_samp))
        {
            worst_fbc = i;
            worst_num_samp = samples;
        }

        XFree(vi);
    }

	auto ret = fbc[best_fbc];
    XFree(fbc);

    xVinfo_ = glXGetVisualFromFBConfig(xDisplay(), ret);
	return ret;
#endif
}
*/
}
