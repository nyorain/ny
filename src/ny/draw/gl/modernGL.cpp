#include <ny/config.h>

#ifdef NY_WithGL

#include <ny/gl/modernGL.hpp>
#include <ny/gl/shader.hpp>
#include <ny/gl/triangulate.hpp>
#include <ny/gl/shader/modernSources.hpp>

#ifdef NY_WithGLBinding
 #include <glbinding/gl/gl.h>
 using namespace gl;
#else
 #include <GL/glew.h>
#endif

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
    static thread_local std::vector<triangle2f> triangles_;

    if(m.empty())
        return;

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    vec2f viewSize(viewport[2], viewport[3]);

    if(!defaultShader.getProgram()) initShader();
    defaultShader.setUniformParameter("inColor", b.rgbaNorm());
    defaultShader.setUniformParameter("viewSize", viewSize);
    defaultShader.use();

    for(const auto& pth : m)
    {
        triangles_.clear();

        if(pth.getPathType() == pathType::rectangle)
        {
            //nyDebug("gl: rectangle pos: ", pth.getRectangle().getPosition(), " size: ", pth.getRectangle().getSize());
            rect2f r(pth.getRectangle().getPosition(), pth.getRectangle().getSize());
            triangles_.emplace_back(r.topLeft(), r.topRight(), r.bottomRight());
            triangles_.emplace_back(r.topLeft(), r.bottomLeft(), r.bottomRight());
        }
        else if(pth.getPathType() == pathType::circle)
        {
            //nyDebug("gl: cricle");
        }
        else if(pth.getPathType() == pathType::text)
        {
            //nyDebug("gl: text");
        }
        else if(pth.getPathType() == pathType::custom)
        {
            //nyDebug("gl: customPath");
            triangles_ = triangulate((float*) pth.getCustom().getBaked().data(), pth.getCustom().getBaked().size());
        }

        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, triangles_.size() * 6 * sizeof(GLfloat), (GLfloat*) triangles_.data(), GL_STATIC_DRAW);

        defaultShader.setUniformParameter("transform", pth.getTransformMatrix());
        GLint posAttrib = glGetAttribLocation(defaultShader.getProgram(), "pos");
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        nyDebug("gl: drawing ", triangles_.size(), " triangles");
        for(auto& tr : triangles_)
            std::cout << tr << "\n";

        glDrawArrays(GL_TRIANGLES, 0, triangles_.size() * 3);

        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
}

void modernGLDrawImpl::stroke(const mask& m, const pen& b)
{

}

}


#endif // NY_WithGL
