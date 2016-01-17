#include <ny/draw/gl/context.hpp>
#include <nytl/log.hpp>
#include <nytl/misc.hpp>

//glcontext backend api
#include <glpbinding/Binding.h>
#include <glbinding/Binding.h>
#include <glesbinding/Binding.h>

#include <glpbinding/glp20/glp.h>
#include <glpbinding/glp30/glp.h>
#include <glbinding/gl43/gl.h>

namespace ny
{

GlContext* GlContext::threadLocalCurrent(bool change, GlContext* newOne)
{
	static thread_local GlContext* current_ = nullptr;
	if(change)
		current_ = newOne;

	return current_;
}

void GlContext::initContext(Api api, unsigned int depth, unsigned int stencil)
{
	api_ = api;
	depthBits_ = depth;
	stencilBits_ = stencil;

	auto* saved = current();
	if(!makeCurrent())
	{
		nytl::sendWarning("GlContext::initContext: failed to make current.");
		return;
	}

	glpbinding::Binding::initialize();
	
	if(api_ == Api::openGL) glbinding::Binding::initialize();
	else if(api_ == Api::openGLES) glesbinding::Binding::initialize();

	//version
	//TODO: Use 3.0 (major, minor) enums to get version on newer contexts
	std::string glver = (const char*) glGetString(glp20::GL_VERSION);

	try
	{
		std::size_t idx;
		majorVersion_ = std::stoi(glver, &idx);
		minorVersion_ = std::stoi(glver.substr(idx));
	}
	catch(const std::exception& err)
	{
		//TODO
		nytl::sendWarning("GlContext::init: invalid GL_VERSION string: ", glver);
		majorVersion_ = 2;
		minorVersion_ = 0;
	}

	//extensions
	if(glpVersion() >= 30)
	{
		using namespace glp30;
		auto number = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &number);

		for(auto i = 0; i < number; ++i)
		{
			std::string ext = (const char*) glGetStringi(GL_EXTENSIONS, i);
			extensions_.push_back(ext);
		}
	}
	else
	{
		using namespace glp20;
		std::string ext = (const char*) glGetString(GL_EXTENSIONS);
		extensions_ = nytl::split(ext, ' ');
	}

	//glsl
	//TODO
	if(api_ == Api::openGL && version() >= 43)
	{
		using namespace gl43;
		auto number = 0;
		glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &number);

		for(auto i = 0; i < number; ++i)
		{
			std::string ver = (const char*) glGetStringi(GL_SHADING_LANGUAGE_VERSION, i);
			try
			{
				std::size_t idx;
				int major = std::stoi(ver, &idx);
				int minor = std::stoi(ver.substr(idx));

				glslVersions_.push_back(major * 10 + minor / 10);
			}
			catch(const std::exception& err)
			{
				nytl::sendWarning("GlContext::init: invalid glsl version string: ", ver);
			}
		}
	}
	else
	{
		using namespace glp20;
		std::string ver = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
		try
		{
			std::size_t idx;
			int major = std::stoi(ver, &idx);
			int minor = std::stoi(ver.substr(idx));

			glslVersions_.push_back(major * 10 + minor / 10);
		}
		catch(const std::exception& err)
		{
			nytl::sendWarning("GlContext::init: invalid glsl version string: ", ver);
		}
	}

	//restore saved one
	if(saved && !saved->makeCurrent())
	{
		nytl::sendWarning("GlContext::initContext: failed to make saved context current again.");
		return;
	}
}

bool GlContext::makeCurrent()
{
	if(makeCurrentImpl())
	{
		threadLocalCurrent(1, this);
		return 1;
	}

	return 0;
}

bool GlContext::makeNotCurrent()
{
	if(!isCurrent()) return 0;

	//TODO: if branch like in makeCurrent?
	threadLocalCurrent(1, nullptr);
	return makeNotCurrentImpl();
}

bool GlContext::isCurrent() const
{
	return (threadLocalCurrent() == this);
}

bool GlContext::glExtensionSupported(const std::string& name) const
{
	for(auto& s : glExtensions())
		if(s == name) return 1;

	return 0;
}

unsigned int GlContext::glpVersion() const
{
	if(api_ == Api::openGL)
	{
		if(version() <= 20) return 0;
		else if(version() < 30) return 20;
		else if(version() < 31) return 30;
		else if(version() < 32) return 31;
		else if(version() >= 32) return 32;
	}
	else if(api_ == Api::openGLES)
	{
		if(version() <= 20) return 0;
		else if(version() < 30) return 20;
		else if(version() < 31) return 30;
		else if(version() < 32) return 31;
		else if(version() >= 32) return 32;
	}

	return 0;
}

}
