#include <ny/gl/modernGL.hpp>
#include <ny/gl/shader.hpp>
#include <ny/gl/shader/modernSources.hpp>

#include <glbinding/gl/gl.h>
using namespace gl;

#include <cassert>

namespace ny
{

//util
namespace
{

//shader
shader defaultShader;

void initShader()
{
    if(!defaultShader.loadFromString(defaultShaderVS, defaultShaderFS))
        nyWarning("failed to init default shader");
}

} //anonymous

//modernGL
void modernGLDrawImpl::clear(color col)
{
    float r,g,b,a;
    col.normalized(r, g, b, a);

    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void modernGLDrawImpl::fill(const mask& m, const brush& b)
{
    const customPath& p = m[0].getCustom();
    size_t pSize = p.size();

    nyDebug(p.size());

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    vec2f viewSize(viewport[2], viewport[3]);

    //nyDebug(viewSize);

    GLfloat* verts = new GLfloat[p.size() * 2];
    for(size_t i(0); i < p.size() * 2; i += 2)
    {
        //auto pos1 = vec3f(p[i / 2].position.x, p[i / 2].position.y, 1.) * p.getTransformMatrix();
        //auto pos = toGL(vec2f(pos1.x, pos1.y), viewSize);
        verts[i] = p[i / 2].position.x;
        verts[i + 1] = p[i / 2].position.y;
    }

    GLuint elements[] = {
        0, 1, 2,
        2, 3, 0
    };


    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, pSize * 2 * sizeof(GLfloat), verts, GL_STATIC_DRAW);

    GLuint ebo;
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    //normalize transaltion for opengl
    /*
    auto tmat = p.getTransformMatrix();
    tmat[0][2] = (tmat[0][2] / viewSize.x) * 2;
    tmat[1][2] = -((tmat[1][2] / viewSize.y)) * 2;

    nyDebug(tmat, "\n\n");
*/

    //nyDebug(p.getTransformMatrix());

    if(!defaultShader.getProgram()) initShader();
    defaultShader.setUniformParameter("inColor", b.rgbaNorm());
    defaultShader.setUniformParameter("transform", p.getTransformMatrix());
    defaultShader.setUniformParameter("viewSize", viewSize);
    defaultShader.use();

    // Specify the layout of the vertex data
    GLint posAttrib = glGetAttribLocation(defaultShader.getProgram(), "pos");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

    //draw
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    delete[] verts;
}

void modernGLDrawImpl::stroke(const mask& m, const pen& b)
{

}

}
