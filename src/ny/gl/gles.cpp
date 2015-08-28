#include <ny/gl/glDrawContext.hpp>

#include <GLES2/gl2.h>

namespace ny
{

void glesDrawImpl::clear(color col)
{
    float r,g,b,a;
    col.normalized(r, g, b, a);

    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void glesDrawImpl::fill(const mask& m, const brush& b)
{

}

void glesDrawImpl::stroke(const mask& m, const pen& b)
{

}

}

