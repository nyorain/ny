#include <ny/config.h>

#ifdef NY_WithGL

#include <ny/gl/glDrawContext.hpp>
#include <ny/surface.hpp>

/*
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;
*/

#include <GL/gl.h>

namespace ny
{

//glDC
glDrawContext::glDrawContext(surface& s, glContext& ctx) : drawContext(s)
{
}

glDrawContext::glDrawContext(surface& s) : drawContext(s)
{
}

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
