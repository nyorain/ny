#include <ny/config.h>

#ifdef NY_WithGL

#include <ny/gl/glDrawContext.hpp>
#include <ny/gl/modernGL.hpp>
#include <ny/surface.hpp>
#include <ny/error.hpp>

#ifdef NY_WithGLBinding
 #include <glbinding/gl/gl.h>
 #include <glbinding/Binding.h>
 using namespace gl;
#else
 #include <GL/glew.h>
#endif //glbinding

#include <string.h>

namespace ny
{

bool isExtensionSupported(const char* extList, const char* extension)
{
    //todo: to c++
    const char *start;
    const char *where, *terminator;

    where = strchr(extension, ' ');
    if (where || *extension == '\0')
        return false;

    for (start=extList;;) {
        where = strstr(start, extension);

        if (!where)
          break;

        terminator = where + strlen(extension);

        if ( where == start || *(where - 1) == ' ' )
          if ( *terminator == ' ' || *terminator == '\0' )
            return true;

        start = terminator;
    }

    return false;
}

bool validGLContext()
{
    return glDrawContext::getCurrent();
}

//glDC
//static
thread_local glDrawContext* glDrawContext::current_;


void glDrawContext::makeContextCurrent(glDrawContext& ctx)
{
    current_ = &ctx;
}

void glDrawContext::makeContextNotCurrent(glDrawContext& ctx)
{
    if(current_ == &ctx)
    {
        current_ = nullptr;
    }
}

glDrawContext* glDrawContext::getCurrent()
{
    return current_;
}

//non-static
glDrawContext::glDrawContext(surface& s) : drawContext(s)
{
}


glDrawContext::~glDrawContext()
{
    if(isCurrent())
        makeContextNotCurrent(*this);
}

//gl
void glDrawContext::init(glApi api, unsigned int depth, unsigned int stencil)
{
    api_ = api;
    depth_ = depth;
    stencil_ = stencil;

    makeCurrentImpl();

    #ifdef NY_WithGLBinding
     glbinding::Binding::initialize();
    #else
     glewExperimental = true;
     if(!glewInit())
     {
         //error...
     }
    #endif

    int maj = 0, min = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &maj);
    glGetIntegerv(GL_MINOR_VERSION, &min);

    major_ = maj;
    minor_ = min;

    if(!impl_)
    {
        if(api_ == glApi::openGL)
        {
            if(major_ >= 3 && minor_ >= 0) impl_.reset(new modernGLDrawImpl()); //better 3.3?
            else impl_.reset(new legacyGLDrawImpl());
        }
        #ifdef NY_WithGLES
        else
        {
            impl_.reset(new glesDrawImpl());
        }
        #endif //GLES
    }

    makeNotCurrentImpl();
}

bool glDrawContext::makeCurrent()
{
    if(isCurrent())
        return 1;

    if(makeCurrentImpl())
    {
        #ifdef NY_WithGLBinding
         glbinding::Binding::useCurrentContext();
        #endif //glbinding

        makeContextCurrent(*this);
        return 1;
    }

    return 0;
}

bool glDrawContext::makeNotCurrent()
{
    if(!isCurrent())
        return 1;

    if(makeNotCurrentImpl())
    {
        makeContextNotCurrent(*this);
        return 1;
    }

    return 0;
}

bool glDrawContext::isCurrent() const
{
    return (current_ == this);
}

bool glDrawContext::assureValid(bool warning) const
{
    if(isCurrent() && impl_)
        return 1;

    if(warning) nyWarning("glDrawContext: glDC not valid: isCurrent: ", isCurrent(), " impl: ", impl_.get(), " ctx: ", this);
    return 0;
}

//dc
void glDrawContext::clear(color col)
{
    if(!assureValid()) return;
    impl_->clear(col);
}

void glDrawContext::updateViewport(vec2ui size)
{
    if(!assureValid()) return;
    glViewport(0, 0, size.x, size.y);
}

rect2f glDrawContext::getClip()
{
    if(!assureValid() || glIsEnabled(GL_SCISSOR_TEST) != GL_TRUE) return rect2f();
    return clip_;
}

void glDrawContext::clip(const rect2f& clip)
{
    if(!assureValid()) return;

    glEnable(GL_SCISSOR_TEST);
    rect2f ccopy = clip;

    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    ccopy.position.y = vp[3] - clip.position.y - clip.size.y;

    glScissor(ccopy.position.x, ccopy.position.y, ccopy.size.x, ccopy.size.y);

    clip_ = clip;
}

void glDrawContext::resetClip()
{
    if(!assureValid()) return;

    clip_ = rect2f(vec2f(0,0), surface_.getSize());
    glDisable(GL_SCISSOR_TEST);
}

void glDrawContext::mask(const customPath& obj)
{
    store_.emplace_back(obj);
}
void glDrawContext::mask(const text& obj)
{
    store_.emplace_back(obj);
}
void glDrawContext::mask(const ny::mask& obj)
{
    store_.insert(store_.cend(), obj.cbegin(), obj.cend());
}
void glDrawContext::mask(const path& obj)
{
    store_.push_back(obj);
}
void glDrawContext::resetMask()
{
    store_.clear();
}
void glDrawContext::fillPreserve(const brush& col)
{
    if(!assureValid() || store_.empty()) return;
    impl_->fill(store_, col);
}
void glDrawContext::strokePreserve(const pen& col)
{
    if(!assureValid() || store_.empty()) return;
    impl_->stroke(store_, col);
}

}

#endif // NY_WithGL
