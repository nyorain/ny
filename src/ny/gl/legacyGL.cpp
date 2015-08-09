#include <ny/gl/glDrawContext.hpp>

#include <glbinding/gl/gl.h>
using namespace gl;

namespace ny
{

void legacyGLDrawImpl::clear(color col)
{
    float r,g,b,a;
    col.normalized(r, g, b, a);

    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void legacyGLDrawImpl::fill(const mask& m, const brush& b)
{

}

void legacyGLDrawImpl::outline(const mask& m, const pen& b)
{

}

}

