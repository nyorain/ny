#include <ny/config.h>

#ifdef NY_WithGL


#include <ny/gl/glDrawContext.hpp>

#ifdef NY_WithGLBinding
 #include <glbinding/gl/gl.h>
 using namespace gl;
#else
 #include <GL/glew.h>
#endif //glbinding

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

void legacyGLDrawImpl::stroke(const mask& m, const pen& b)
{

}

}

#endif // NY_WithGL
