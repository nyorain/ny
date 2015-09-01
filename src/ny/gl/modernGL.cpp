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

vec2f toGL(const vec2f& src, const vec2ui& size)
{
    return ((vec2f(src.x, size.y - src.y) / size) * 2) - vec2f(1, 1);
    //return src / size;
}

/*
rect2f toGL(const rect2f& src, const vec2ui& size)
{
    rect2f ret;

    ret.position = (vec2f(src.position.x, size.y - src.position.y - src.size.y) / size) * 2 - 1;
    ret.size = (src.size / size) * 2;

    return ret;
}
*/

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

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    vec2ui viewSize(viewport[2], viewport[3]);


    GLfloat* verts = new GLfloat[p.size() * 2];
    for(size_t i(0); i < p.size() * 2; i += 2)
    {
        auto pos1 = vec3f(p[i / 2].position.x, p[i / 2].position.y, 1.) * p.getTransformMatrix();
        auto pos = toGL(vec2f(pos1.x, pos1.y), viewSize);
        verts[i] = pos.x;
        verts[i + 1] = pos.y;
        nyDebug(pos);
    }
    nyDebug("\n\n");

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
    if(!defaultShader.getProgram()) initShader();
    defaultShader.setUniformParameter("inColor", b.rgbaNorm());
    //defaultShader.setUniformParameter("transform", tmat);
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
