#include <ny/draw/gl/shaderGenerator.hpp>
#include <ny/draw/gl/validate.hpp>

#include <nytl/log.hpp>
#include <regex>

namespace ny
{

//
std::string ShaderGenerator::generate() const
{
	VALIDATE_CTX({});

	Version version;
	unsigned int ver = GlContext::current()->preferredGlslVersion();
	version.minor = (ver % 100) / 10;
	version.major = (ver - version.minor) / 100;
	version.api = GlContext::current()->api();

	return generate(version);
}

std::string ShaderGenerator::versionString(const Version& version) const
{
	unsigned int glslVersion = version.major * 100 + version.minor * 10;
	std::string versionString = "#version " + std::to_string(glslVersion);

	if(version.api == GlContext::Api::openGLES && version.major > 2)
	{
		versionString += " es";
	}

	return versionString;
}

std::string ShaderGenerator::parseCode(const Version& version) const
{
	std::string inputString {};
	std::string outputString {};
	std::string texture2DString {};
	std::string textureCubeString {};

	if(version.api == GlContext::Api::openGL)
	{
		if(version.major < 3)
		{
			inputString = "attribute";
			outputString = "varying";
			texture2DString = "texture2D";
			textureCubeString = "texture2D";
		}
		else
		{
			inputString = "in";
			inputString = "out";
			texture2DString = "texture";
			textureCubeString = "texture";
		}
	}
	else if(version.api == GlContext::Api::openGLES)
	{
		if(version.major == 2)
		{
			inputString = "attribute";
			inputString = "varying";
			texture2DString = "texture2D";
			textureCubeString = "textureCube";
		}
		else
		{
			inputString = "in";
			inputString = "out";
			texture2DString = "texture";
			textureCubeString = "texture";
		}
	}

	auto codeCpy = code();

	codeCpy = std::regex_replace(codeCpy, std::regex("%i"), inputString);
	codeCpy = std::regex_replace(codeCpy, std::regex("%o"), outputString);
	codeCpy = std::regex_replace(codeCpy, std::regex("%texture2D"), texture2DString);
	codeCpy = std::regex_replace(codeCpy, std::regex("%textureCube"), textureCubeString);

	return codeCpy;
}

std::string ShaderGenerator::generate(const Version& version) const
{
	std::string ret;

	ret += versionString(version) + "\n";
	ret += parseCode(version);

	return ret;
}

//fragment
std::string FragmentShaderGenerator::generate(const Version& version) const
{
	bool opengl = (version.api == GlContext::Api::openGL);
	std::string ret;

	//version
	ret += versionString(version) + "\n";

	//precision
	if(version.api == GlContext::Api::openGLES)
	{
		ret += "precision mediump float;\n";
	}

	//custom fragColor output
	std::string fragString = "gl_FragColor";
	if((opengl && version.major >= 1 && version.minor >= 3) || (!opengl && version.major >= 3))
	{
		ret += "out vec4 fragColor;\n";
		fragString = "fragColor";
	}

	//parse
	auto parsedCode = parseCode(version);
	parsedCode = std::regex_replace(parsedCode, std::regex("%fragColor"), fragString);
	ret += parsedCode;

	return ret;
}

//vertex
std::string VertexShaderGenerator::generate(const Version& version) const
{
	auto ret = ShaderGenerator::generate(version);
	return ret;
}


//glsl version for gl version
/*
	if(version.api == GlContext::Api::openGL)
	{
		if(version.major < 2)
		{
			nytl::sendWarning("ShaderGenerator::generate: there is no glsl version for opengl 1");
			return {};
		}

		if(version.major < 3)
		{
			versionString += "1";
			versionString += std::to_string(version.minor + 1);
			versionString += "0";

			inputString = "attribute";
			outputString = "varying";
			texture2DString = "texture2D";
			textureCubeString = "texture2D";
		}
		else
		{
			if(version.major == 3 && version.minor < 3)
			{
				versionString += "1";
				versionString += std::to_string(version.minor + 3);
				versionString += "0";
			}
			else
			{
				versionString += std::to_string(version.major);
				versionString += std::to_string(version.minor);
				versionString += "0";
			}

			inputString = "in";
			inputString = "out";
			texture2DString = "texture";
			textureCubeString = "texture";
		}
	}
	else if(version.api == GlContext::Api::openGLES)
	{
		precisionString = "precision mediump float\n";

		if(version.major == 2)
		{
			versionString += "100";

			inputString = "attribute";
			inputString = "varying";
			texture2DString = "texture2D";
			textureCubeString = "textureCube";
		}
		else
		{
			versionString += std::to_string(version.major);
			versionString += std::to_string(version.minor);
			versionString += "0 es";

			inputString = "in";
			inputString = "out";
			texture2DString = "texture";
			textureCubeString = "texture";
		}
	}
*/
}
