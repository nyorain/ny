#include <ny/config.h>

#ifdef NY_WithGL

#include <ny/gl/glContext.hpp>

#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>
using namespace gl;

#include <string.h>

namespace ny
{

bool isExtensionSupported(const char* extList, const char* extension)
{
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


//glContext
//static
std::map<std::thread::id, glContext*> glContext::current_;

void glContext::makeContextCurrent(glContext& ctx)
{
    current_[std::this_thread::get_id()] = &ctx;
}

void glContext::makeContextNotCurrent(glContext& ctx)
{
    if(&ctx == current_[std::this_thread::get_id()])
    {
        current_[std::this_thread::get_id()] = nullptr;
    }
}

bool glContext::valid()
{
    if(current_[std::this_thread::get_id()])
        return 1;

    return 0;
}

glContext* glContext::getCurrent()
{
    return current_[std::this_thread::get_id()];
}

//individual
glContext::glContext()
{
}

glContext::~glContext()
{
    if(isCurrent())
        makeContextNotCurrent(*this);
}

void glContext::init(glApi api, unsigned int depth, unsigned int stencil)
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

bool glContext::makeCurrent()
{
    if(makeCurrentImpl())
    {
        glbinding::Binding::useCurrentContext();
        makeContextCurrent(*this);
        return 1;
    }

    return 0;
}

bool glContext::makeNotCurrent()
{
    if(makeNotCurrentImpl())
    {
        glbinding::Binding::useCurrentContext();
        makeContextNotCurrent(*this);
        return 1;
    }

    return 0;
}

bool glContext::isCurrent()
{
    return (getCurrent() == this);
}

}

#endif //WithGL
