#include <ny/draw/gl/context.hpp>
#include <ny/base/log.hpp>
#include <nytl/misc.hpp>
#include <EGL/egl.h>

#include <ny/draw/gl/glad/glad.h>

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
		throw std::runtime_error("GlContext::initContext: failed to make context current");
		return;
	}

	//may be a problem for opengl es
	if(!gladLoadGL())
	{
		throw std::runtime_error("GlContext::initContext: failed to load opengl");
	}

	//version
	majorVersion_ = GLVersion.major;
	minorVersion_ = GLVersion.minor;

	//extensions
	if(glpVersion() >= 30)
	{
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
		std::string ext = (const char*) glGetString(GL_EXTENSIONS);
		extensions_ = nytl::split(ext, ' ');
	}

	//glsl
	//TODO
	if(api_ == Api::openGL && version() >= 43)
	{
		auto number = 0;
		glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &number);

		for(auto i = 0; i < number; ++i)
		{
			std::string ver = (const char*) glGetStringi(GL_SHADING_LANGUAGE_VERSION, i);
			try
			{
				auto verSub = ver.substr(ver.find_first_of('.') - 1, std::string::npos);

				std::size_t idx;
				int major = std::stoi(verSub, &idx);
				int minor = std::stoi(verSub.substr(idx + 1));

				glslVersions_.push_back(major * 10 + minor / 10);
			}
			catch(const std::exception& err)
			{
				sendWarning("GlContext::init: invalid glsl version string: ", ver);
			}
		}
	}
	else
	{
		std::string ver = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
		try
		{
			auto verSub = ver.substr(ver.find_first_of('.') - 1, std::string::npos);

			std::size_t idx;
			int major = std::stoi(verSub, &idx);
			int minor = std::stoi(verSub.substr(idx + 1));

			glslVersions_.push_back(major * 10 + minor / 10);
		}
		catch(const std::exception& err)
		{
			sendWarning("GlContext::init: invalid glsl version string: ", ver);
		}
	}

	//TODO: choose highest if multiple are available
	preferredGlslVersion_ = glslVersions_[0];

	//restore saved one
	if(saved && !saved->makeCurrent())
	{
		sendWarning("GlContext::initContext: failed to make saved context current again.");
		return;
	}
}

bool GlContext::makeCurrent()
{
	if(isCurrent()) return 1;

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

bool GlContext::sharedWith(const GlContext& other) const
{
	for(auto& ctx : sharedContexts())
		if(ctx == &other) return 1;

	return 0;
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

void GlContext::updateViewport(const rect2f& viewport)
{
	if(!current())
	{
		sendWarning("GlContext::updateViewport called with not-current context");
		return;
	}

	glViewport(viewport.position.x, viewport.position.y, viewport.size.x, viewport.size.y);
}

bool GlContext::apply()
{
	glFinish();
	return 1;
}

}
