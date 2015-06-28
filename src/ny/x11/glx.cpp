#include <ny/config.h>

#ifdef NY_WithGL

#include <ny/backends/x11/glx.hpp>

#include <ny/backends/x11/appContext.hpp>

#include <ny/graphics/gl/glDC.hpp>
#include <ny/app/error.hpp>

//from X11, not compatible with glbindijng from include/gl.h
#ifdef None
#undef None
const unsigned char None = 0;
#endif // None

#include <glbinding/Binding.h>
#include <GL/glx.h>

namespace ny
{

struct glxFBC
{
    GLXFBConfig config;
};

struct glxc
{
    GLXContext context;
};

//errorHandler
bool errorOccured = 0;
int ctxErrorHandler(Display *display, XErrorEvent *ev)
{
    errorOccured = true;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
glxContext::glxContext(Drawable d, glxFBC* conf) : drawable_(d)
{
    glxContext_ = new glxc;

    GLXFBConfig fbc = conf->config;
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

    glContext::init(glApi::openGL, depth, stencil);
}

glxContext::~glxContext()
{
    glXDestroyContext(getXDisplay(), glxContext_->context);
}

bool glxContext::makeCurrent()
{
    if(!glXMakeCurrent(getXDisplay(), drawable_, glxContext_->context))
        return 0;

    glbinding::Binding::useCurrentContext();
    makeContextCurrent(*this);

    return 1;
}

bool glxContext::makeNotCurrent()
{
    if(!glXMakeCurrent(getXDisplay(), 0, nullptr))
        return 0;

    makeContextNotCurrent(*this);
    return 1;
}

//x11GL/////////////////////////////////////////////////////////////////////////////
glxWindowContext::glxWindowContext(x11WindowContext& wctx) : wc_(wctx)
{
}

glxWindowContext::~glxWindowContext()
{
    if(fbc_) delete fbc_;
    if(glContext_) delete glContext_;
    if(drawContext_) delete drawContext_;
}

XVisualInfo* glxWindowContext::initFBConfig(const glxWindowContextSettings& s)
{
    const int attribs[] =
    {
      GLX_X_RENDERABLE    , True,
      GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
      GLX_RENDER_TYPE     , GLX_RGBA_BIT,
      GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
      GLX_RED_SIZE        , 8,
      GLX_GREEN_SIZE      , 8,
      GLX_BLUE_SIZE       , 8,
      GLX_ALPHA_SIZE      , 8,
      GLX_DEPTH_SIZE      , (int) s.depth,
      GLX_STENCIL_SIZE    , (int) s.stencil,
      GLX_DOUBLEBUFFER    , True,
      //GLX_SAMPLE_BUFFERS  , 1,
      //GLX_SAMPLES         , 4,
      None
    };


    int glxMajor, glxMinor;
    if (!glXQueryVersion(getXDisplay(), &glxMajor, &glxMinor) || ((glxMajor == 1) && (glxMinor < 3) ) || (glxMajor < 1)) //glx must be > 1.3
    {
        throw std::runtime_error("Invalid glx version. glx Version must be > 1.3");
        return nullptr;
    }

    int fbcount = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(getXDisplay(), DefaultScreen(getXDisplay()), attribs, &fbcount);
    if (!fbc || !fbcount)
    {
        throw std::runtime_error("failed to retrieve fbconfig");
        return nullptr;
    }

    //get the config with the most samples
    int best_fbc = -1, worst_fbc = -1, best_num_samp = 0, worst_num_samp = 0;
    for(int i(0); i < fbcount; i++)
    {
        XVisualInfo *vi = glXGetVisualFromFBConfig(getXDisplay(), fbc[i]);

        if(!vi)
        {
            XFree(vi);
            continue;
        }

        int samp_buf, samples;
        glXGetFBConfigAttrib(getXDisplay(), fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
        glXGetFBConfigAttrib(getXDisplay(), fbc[i], GLX_SAMPLES       , &samples  );

        if ( best_fbc < 0 || (samp_buf && samples > best_num_samp))
        {
            best_fbc = i;
            best_num_samp = samples;
        }

        if ( worst_fbc < 0 || (!samp_buf || samples < worst_num_samp))
        {
            worst_fbc = i;
            worst_num_samp = samples;
        }

        XFree(vi);
    }

    fbc_ = new glxFBC;
    fbc_->config = fbc[best_fbc];
    XFree(fbc);

    return glXGetVisualFromFBConfig(getXDisplay(), fbc_->config);
}


void glxWindowContext::init()
{
    glContext_ = new glxContext(wc_.getXWindow(), fbc_);
    drawContext_ = new glDrawContext((surface&)wc_.getWindow(), *glContext_);
}


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

}

#endif // NY_WithGL
