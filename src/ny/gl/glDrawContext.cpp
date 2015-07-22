#include <ny/config.h>

#ifdef NY_WithGL

#include <ny/gl/glDrawContext.hpp>
#include <ny/surface.hpp>

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

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
    return (glDrawContext::getCurrent());
}

//glDC
//static
std::map<std::thread::id, glDrawContext*> glDrawContext::current_;


void glDrawContext::makeContextCurrent(glDrawContext& ctx)
{
    current_[std::this_thread::get_id()] = &ctx;
}

void glDrawContext::makeContextNotCurrent(glDrawContext& ctx)
{
    if(&ctx == current_[std::this_thread::get_id()])
    {
        current_[std::this_thread::get_id()] = nullptr;
    }
}

glDrawContext* glDrawContext::getCurrent()
{
    auto it = current_.find(std::this_thread::get_id());
    if(it == current_.end())
        return nullptr;

    return it->second;
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

    makeCurrent();
    glbinding::Binding::initialize();

    int maj = 0, min = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &maj);
    glGetIntegerv(GL_MINOR_VERSION, &min);

    major_ = maj;
    minor_ = min;

    makeNotCurrent();
}

bool glDrawContext::makeCurrent()
{
    if(makeCurrentImpl())
    {
        glbinding::Binding::useCurrentContext();
        makeContextCurrent(*this);
        return 1;
    }

    return 0;
}

bool glDrawContext::makeNotCurrent()
{
    if(makeNotCurrentImpl())
    {
        glbinding::Binding::useCurrentContext();
        makeContextNotCurrent(*this);
        return 1;
    }

    return 0;
}

bool glDrawContext::isCurrent()
{
    return (getCurrent() == this);
}

//dc
void glDrawContext::clear(color col)
{
    float r, g, b, a = 0;
    col.normalized(r, g, b, a);

    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

rect2f glDrawContext::getClip()
{
    if(glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE) return clip_;

    return rect2f();
}

void glDrawContext::clip(const rect2f& clip)
{
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
    clip_ = rect2d(vec2d(0,0), surface_.getSize());
    glDisable(GL_SCISSOR_TEST);
}

}

#endif // NY_WithGL
