#include <ny/config.h>

#ifdef NY_WithGL

#include <ny/x11/glx.hpp>
#include <ny/x11/x11WindowContext.hpp>
#include <ny/x11/x11AppContext.hpp>

#include <ny/gl/glDrawContext.hpp>
#include <ny/error.hpp>
#include <ny/window.hpp>

#include <GL/glx.h>

namespace ny
{

struct glxc
{
    GLXContext context;
};

struct glxFBC
{
    GLXFBConfig config;
};


//errorHandler
bool errorOccured = 0;
int ctxErrorHandler(Display *display, XErrorEvent *ev)
{
    errorOccured = true;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
glxDrawContext::glxDrawContext(const x11WindowContext& wc) : glDrawContext(wc.getWindow()), wc_(wc)
{
    GLXFBConfig fbc = wc.getGLXFBC()->config;
    if(!fbc)
        return;

    int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

    typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );
    const char *glxExts = glXQueryExtensionsString(getXDisplay(), DefaultScreen(getXDisplay()));

    if(isExtensionSupported(glxExts,"GLX_ARB_create_context") && glXCreateContextAttribsARB) //supported
    {
        int contextAttribs[] =
        {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 3,
            //GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            None
        };

        glxContext_->context = glXCreateContextAttribsARB(getXDisplay(), fbc, nullptr, True, contextAttribs );
        XSync(getXDisplay(), False);
        if(!glxContext_->context|| errorOccured)
        {
            errorOccured = 0;
            glxContext_->context = nullptr;
            sendWarning("modern GL context could not be created, creating legacy GL context");
        }

        if(!glxContext_->context)
        {
            //legacy
            contextAttribs[1] = 1;
            contextAttribs[3] = 0;

            glxContext_->context = glXCreateContextAttribsARB(getXDisplay(), fbc, nullptr, True, contextAttribs );
            XSync(getXDisplay(), False);
            if(!glxContext_->context || errorOccured)
            {
                errorOccured = 0;
                glxContext_->context = nullptr;
                sendWarning("legacy GL context could not be created, trying old method");
            }
        }
    }

    if(!glxContext_->context)
    {
        //glxContext_->context = glXCreateNewContext(getXDisplay(), fbc->config, GLX_RGBA_TYPE, 0, True);
    }


    XSetErrorHandler(oldHandler);

    if(!glxContext_->context || errorOccured)
    {
        throw std::runtime_error("could not create glx Context");
        return;
    }

    int depth, stencil = 0;
    glXGetFBConfigAttrib(getXDisplay(), fbc, GLX_STENCIL_SIZE, &stencil);
    glXGetFBConfigAttrib(getXDisplay(), fbc, GLX_DEPTH_SIZE, &depth);

    init(glApi::openGL, depth, stencil);
}

glxDrawContext::~glxDrawContext()
{
    if(glxContext_ && glxContext_->context) glXDestroyContext(getXDisplay(), glxContext_->context);
    if(glxContext_) delete glxContext_;
}

bool glxDrawContext::makeCurrentImpl()
{
    if(!glXMakeCurrent(getXDisplay(), wc_.getXWindow(), glxContext_->context))
        return 0;

    return 1;
}

bool glxDrawContext::makeNotCurrentImpl()
{
    if(!glXMakeCurrent(getXDisplay(), 0, nullptr))
        return 0;

    return 1;
}

bool glxDrawContext::swapBuffers()
{
    glXSwapBuffers(getXDisplay(), wc_.getXWindow());
    return 1;
}


/*
//toplevel
glxToplevelWindowContext::glxToplevelWindowContext(toplevelWindow& win, const glxWindowContextSettings& s) : windowContext((window&)win, s), x11ToplevelWindowContext(win, s, 0), glxWindowContext((x11WindowContext&)*this)
{
    if(!(xVinfo_ = initFBConfig(s)))
    {
        return;
    }

    x11WindowContext::create(InputOutput);

    init();
}

drawContext& glxToplevelWindowContext::beginDraw()
{
    glContext_->makeCurrent(); //glXMakeCurrent(xDisplay_, xWindow_, glContext_);
    return *drawContext_;
}

void glxToplevelWindowContext::finishDraw()
{
    drawContext_->apply();
    glXSwapBuffers(xDisplay_, xWindow_);
    XFlush(xDisplay_);
}

void glxToplevelWindowContext::setSize(vec2ui size, bool change)
{
    x11ToplevelWindowContext::setSize(size, change);
    //glXMakeCurrent(xDisplay_, xWindow_, glContext_);
    //glViewport(0, 0, size.x, size.y);
}

//child
glxChildWindowContext::glxChildWindowContext(childWindow& win, const glxWindowContextSettings& s) : windowContext((window&)win, s), x11ChildWindowContext(win, s, 0), glxWindowContext((x11WindowContext&)*this)
{
    GLint attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    xVinfo_ = glXChooseVisual(xDisplay_, 0, attribs);

    x11WindowContext::create(InputOutput);

    //glContext_ = glXCreateContext(xDisplay_, xVinfo_, NULL, GL_TRUE);
    //glXMakeCurrent(xDisplay_, xWindow_, glContext_);
}

drawContext& glxChildWindowContext::beginDraw()
{
    //glXMakeCurrent(xDisplay_, xWindow_, glContext_);
    return *drawContext_;
}

void glxChildWindowContext::finishDraw()
{
    drawContext_->apply();
    glXSwapBuffers(xDisplay_, xWindow_);
    XFlush(xDisplay_);
}

void glxChildWindowContext::setSize(vec2ui size, bool change)
{
    x11ChildWindowContext::setSize(size, change);
    //glXMakeCurrent(xDisplay_, xWindow_, glContext_);
    //glViewport(0, 0, size.x, size.y);
}
*/

}

#endif // NY_WithGL
