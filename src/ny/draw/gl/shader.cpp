#include <ny/draw/gl/shader.hpp>
#include <ny/draw/gl/glContext.hpp>
#include <ny/draw/color.hpp>

#include <nytl/log.hpp>

#include <glpbinding/glp20/glp.h>
using namespace glp20;

#include <fstream>

//macro for current context validation
#if defined(__GNUC__) || defined(__clang__)
 #define FUNC_NAME __PRETTY_FUNCTION__
#else
 #define FUNC_NAME __func__
#endif //FUNCNAME

#define VALIDATE_CTX(...) if(!validContext())\
	{ nytl::sendWarning(FUNC_NAME, ": invalid GlContext for GlResource "); return __VA_ARGS__; }

namespace ny
{

//shader
Shader::Shader()
{
}

Shader::~Shader()
{
	reset();
}

void Shader::reset()
{
	if(program_)
	{
		if(!validContext())
		{
			nytl::sendWarning("Shader::reset: Inalid current opengl context");
			return;
		}

		glDeleteProgram(program_);
	}
}

bool Shader::loadFromFile(const std::string& vertexFile, const std::string& fragmentFile)
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
    }
	else
	{
		nytl::sendWarning("Shader::loadFromFile: failed to open fragment file ", vertexFile);
		return 0;
	}

    std::ifstream fragStream;
    fragStream.open(fragmentFile);
    if(fragStream.is_open())
    {
        std::string tmp = "";

        while(std::getline(fragStream, tmp))
            fragString += tmp + "\n";
    }
	else
	{
		nytl::sendWarning("Shader::loadFromFile: failed to open fragment file ", fragmentFile);
		return 0;
	}

    return compile(vertString, fragString);
}
bool Shader::loadFromFile(const std::string& file, Shader::Type type)
{
    std::string source;
    std::string tmp = "";
    std::ifstream stream;
    stream.open(file);

    if(!stream.is_open())
    {
		nytl::sendWarning("Shader::loadFromFile: failed to open file ", file);
		return 0;
    }

    while(std::getline(stream, tmp))
		source += tmp + "\n";

    if(type == Type::vertex) return compile(source, std::string());
    else if(type == Type::fragment) return compile(std::string(), source);

	return 0;
}

bool Shader::loadFromString(const std::string& vertexShader, const std::string& fragmentShader)
{
    return compile(vertexShader, fragmentShader);
}

bool Shader::loadFromString(const std::string& shader, Shader::Type type)
{
    if(type == Type::vertex) return compile(shader, std::string());
    else if(type == Type::fragment) return compile(std::string(), shader);

    return 0;
}

void Shader::uniform(const std::string& name, float value)
{
	VALIDATE_CTX();

    int location = glGetUniformLocation(program_, name.c_str());
    glUniform1f(location, value);
}

void Shader::uniform(const std::string& name, float x, float y)
{
	VALIDATE_CTX();

    int location = glGetUniformLocation(program_, name.c_str());
    glUniform2f(location, x, y);
}
void Shader::uniform(const std::string& name, float x, float y, float z)
{
	VALIDATE_CTX();

    int location = glGetUniformLocation(program_, name.c_str());
    glUniform3f(location, x, y, z);
}
void Shader::uniform(const std::string& name, float x, float y, float z, float w)
{
	VALIDATE_CTX();

    int location = glGetUniformLocation(program_, name.c_str());
    glUniform4f(location, x, y, z, w);
}
void Shader::uniform(const std::string& name, const vec2f& value)
{
    uniform(name, value.x, value.y);
}
void Shader::uniform(const std::string& name, const vec3f& value)
{
    uniform(name, value.x, value.y, value.z);
}
void Shader::uniform(const std::string& name, const vec4f& value)
{
    uniform(name, value.x, value.y, value.z, value.w);
}
void Shader::uniform(const std::string& name, const mat2f& value)
{
	VALIDATE_CTX();

    int location = glGetUniformLocation(program_, name.c_str());
    glUniformMatrix2fv(location, 1, GL_FALSE, value.data());
}
void Shader::uniform(const std::string& name, const mat3f& value)
{
	VALIDATE_CTX();

    int location = glGetUniformLocation(program_, name.c_str());
    glUniformMatrix3fv(location, 1, GL_FALSE, value.data());
}
void Shader::uniform(const std::string& name, const mat4f& value)
{
	VALIDATE_CTX();

    int location = glGetUniformLocation(program_, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, value.data());
}

bool Shader::compile(const std::string& vertexShader, const std::string& fragmentShader)
{
	if(!GlContext::currentValid())
	{
		nytl::sendWarning("Shader::compile: no valid current GlContext");
		return 0;
	}

	reset();

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
			nytl::sendWarning("failed to compile vertex shader:\n\t", info.data());
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
			nytl::sendWarning("failed to compile fragment shader:\n\t", info.data());
            fsID = 0;
        }
    }

    if(!vsID & !fsID) return 0;

    unsigned int progID = glCreateProgram();
    if(vsID) glAttachShader(progID, vsID);
    if(fsID) glAttachShader(progID, fsID);
    glLinkProgram(progID);

    if(vsID) glDeleteShader(vsID);
    if(fsID) glDeleteShader(fsID);

    program_ = progID;
	GlResource::glContext(*GlContext::current());

    return 1;
}

void Shader::use() const
{
	VALIDATE_CTX();

    if(program_) glUseProgram(program_);
	else nytl::sendWarning("Shader::use: shader has no compiled program");
}


}

