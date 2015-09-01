#include <ny/config.h>

#ifdef NY_WithGL

#include <ny/gl/shader.hpp>
#include <ny/error.hpp>
#include <nyutil/mat.hpp>

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

#include <fstream>

namespace ny
{

bool validGLContext();

//shader
shader::shader()
{
}

shader::~shader()
{
}

bool shader::loadFromFile(const std::string& vertexFile, const std::string& fragmentFile)
{
    std::string vertString;
    std::string fragString;

    std::ifstream vertStream;
    vertStream.open(vertexFile);
    if(vertStream.is_open())
    {
        std::string tmp = "";

        while(std::getline(vertStream, tmp))
            vertString += tmp + "\n";

        vertStream.close();
    }

    std::ifstream fragStream;
    fragStream.open(fragmentFile);
    if(fragStream.is_open())
    {
        std::string tmp = "";

        while(std::getline(fragStream, tmp))
            fragString += tmp + "\n";

        fragStream.close();
    }

    return compile(vertString, fragString);
}
bool shader::loadFromFile(const std::string& file, type type)
{
    std::string source;
    std::string tmp = "";
    std::ifstream stream;
    stream.open(file);

    if(!stream.is_open())
    {
        return 0;
    }

    while(std::getline(stream, tmp))
    source += tmp + "\n";
    stream.close();

    if(type == vertex) return compile(source, std::string());
    if(type == fragment) return compile(std::string(), source);

    return 0;
}

bool shader::loadFromString(const std::string& vertexShader, const std::string& fragmentShader)
{
    return compile(vertexShader, fragmentShader);
}
bool shader::loadFromString(const std::string& shader, type type)
{
    if(type == vertex) return compile(shader, std::string());
    if(type == fragment) return compile(std::string(), shader);

    return 0;
}

void shader::setUniformParameter(const std::string& name, float value)
{
    int location = glGetUniformLocation(program_, name.c_str());
    glUniform1f(location, value);
}

void shader::setUniformParameter(const std::string& name, float x, float y)
{
    int location = glGetUniformLocation(program_, name.c_str());
    glUniform2f(location, x, y);
}
void shader::setUniformParameter(const std::string& name, float x, float y, float z)
{
    int location = glGetUniformLocation(program_, name.c_str());
    glUniform3f(location, x, y, z);
}
void shader::setUniformParameter(const std::string& name, float x, float y, float z, float w)
{
    int location = glGetUniformLocation(program_, name.c_str());
    glUniform4f(location, x, y, z, w);
}
void shader::setUniformParameter(const std::string& name, const vec2f& value)
{
    setUniformParameter(name, value.x, value.y);
}
void shader::setUniformParameter(const std::string& name, const vec3f& value)
{
    setUniformParameter(name, value.x, value.y, value.z);
}
void shader::setUniformParameter(const std::string& name, const vec4f& value)
{
    setUniformParameter(name, value.x, value.y, value.z, value.w);
}
void shader::setUniformParameter(const std::string& name, const mat2f& value)
{

}
void shader::setUniformParameter(const std::string& name, const mat3f& value)
{
    int location = glGetUniformLocation(program_, name.c_str());
    glUniformMatrix3fv(location, 1, GL_FALSE, value.ptr());
}
void shader::setUniformParameter(const std::string& name, const mat4f& value)
{

}
void shader::setUniformParameter(const std::string& name, const color& value)
{
}

bool shader::compile(const std::string& vertexShader, const std::string& fragmentShader)
{
    unsigned int vsID = 0;
    unsigned int fsID = 0;

    if(!vertexShader.empty())
    {
        vsID = glCreateShader(GL_VERTEX_SHADER);
        GLint result = 0;

        const char* const str = vertexShader.c_str();
        glShaderSource(vsID, 1, &str, nullptr);
        glCompileShader(vsID);

        glGetShaderiv(vsID, GL_COMPILE_STATUS, &result);
        if(result != 1)
        {
            int infoLength;
            glGetShaderiv(vsID, GL_INFO_LOG_LENGTH, &infoLength);
            std::vector<char> info(infoLength);
            glGetShaderInfoLog(vsID, infoLength, nullptr, info.data());
            nyWarning("failed to compile vertex shader:\n", info.data());
            vsID = 0;
        }
    }

    if(!fragmentShader.empty())
    {
        fsID = glCreateShader(GL_FRAGMENT_SHADER);
        GLint result = 0;

        const char* const str = fragmentShader.c_str();
        glShaderSource(fsID, 1, &str, nullptr);
        glCompileShader(fsID);

        glGetShaderiv(fsID, GL_COMPILE_STATUS, &result);
        if(result != 1)
        {
            int infoLength;
            glGetShaderiv(fsID, GL_INFO_LOG_LENGTH, &infoLength);
            std::vector<char> info(infoLength);
            glGetShaderInfoLog(fsID, infoLength, nullptr, info.data());
            nyWarning("failed to compile fragment shader:\n", info.data());
            fsID = 0;
        }
    }

    if(!vsID & !fsID) return 0;

    unsigned int progID = glCreateProgram();
    if(vsID)glAttachShader(progID, vsID);
    if(fsID)glAttachShader(progID, fsID);
    glLinkProgram(progID);

    if(vsID)glDeleteShader(vsID);
    if(fsID)glDeleteShader(fsID);

    program_ = progID;
    return 1;
}

void shader::use() const
{
    if(program_ && validGLContext())
        glUseProgram(program_);
}


}

#endif // NY_WithGL
