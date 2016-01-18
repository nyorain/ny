#include <ny/backend/x11/glx.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/backend/x11/appContext.hpp>

#include <nytl/misc.hpp>
#include <nytl/log.hpp>

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
		nytl::sendWarning("GlxContext::GlxContext: Error occured");
	    errorOccured = true;
	    return 0;
	}
}

//
GlxContext::GlxContext(X11WindowContext& wc, GLXFBConfig fbc) : wc_(&wc)
{
    int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);
    using procType = GLXContext(*)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

    procType glXCreateContextAttribsARB = 
		(procType) glXGetProcAddressARB((const GLubyte*) "glXCreateContextAttribsARB" );

    const char* glxExts = glXQueryExtensionsString(xDisplay(), DefaultScreen(xDisplay()));
	auto extVec = split(glxExts, ' ');
	
	nytl::sendLog("glx extensions: ", glxExts);
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
			nytl::sendWarning("modern GL context could not be created, creating legacy GL context");
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
				nytl::sendWarning("legacy GL context could not be created, trying old method");
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

    initContext(Api::openGL, depth, stencil);
}

GlxContext::~GlxContext()
{
    if(glxContext_) glXDestroyContext(xDisplay(), glxContext_);
}

bool GlxContext::makeCurrentImpl()
{
    if(!glXMakeCurrent(xDisplay(), wc_->xWindow(), glxContext_))
    {
		nytl::sendWarning("Glx::makeCurrentImpl: glxmakecurrent failed");
        return 0;
    }

    return 1;
}

bool GlxContext::makeNotCurrentImpl()
{
    if(!glXMakeCurrent(xDisplay(), 0, nullptr))
    {
		nytl::sendWarning("Glx::makeNotCurrentImpl: glxmakecurrent failed");
        return 0;

    }

    return 1;
}

bool GlxContext::apply()
{
	drawContext_.apply();
    glXSwapBuffers(xDisplay(), wc_->xWindow());
    return 1;
}

void GlxContext::size(const vec2ui& size)
{
	makeCurrent();
	updateViewport(rect2f({0.f, 0.f}, size));
}

}
