#include <ny/draw/gl/context.hpp>
#include <ny/base/log.hpp>
#include <nytl/misc.hpp>

#include <ny/draw/gl/glad/glad.h>

namespace ny
{

namespace
{

//utiltity for loading gl and gles
const GlContext* gCtx = nullptr;
void* loadCallback(const char* name)
{
	if(!gCtx) return nullptr;
	return gCtx->procAddr(name);
}

//parsing shader version
GlContext::GlslVersion parseGlslVersion(const std::string& name)
{
	GlContext::GlslVersion version;
	version.api = GlContext::Api::gl;

	auto pos = name.find(" es");
	if(pos != std::string::npos)
	{
		auto next = pos + 3;
		if(next >= name.size() || name[next] == ' ') version.api = GlContext::Api::gles;
	}

	pos = name.find(" ES");
	if(pos != std::string::npos)
	{
		auto next = pos + 3;
		if(next >= name.size() || name[next] == ' ') version.api = GlContext::Api::gles;
	}


	pos = 0u;
	while(!std::isdigit(name[pos], std::locale()) && pos < name.size()) pos++;
	if(pos == name.size())
	{
		sendWarning("GlContext::init: invalid glsl version string: '", name);
		return version;
	}

	int major, minor;
	auto count = std::sscanf(name.substr(pos).c_str(), "%d.%d", &major, &minor);

	if(count == 1)
	{
		if(major >= 100) //e.g. just 330 for version 3.3
		{
			version.major = major / 100;
			version.minor = (major % 100) / 10;
		}
		else  //e.g. 33
		{
			version.major = major / 10;
			version.minor = (major % 10) / 10;
		}
	}
	else if(count == 2)
	{
		if(minor >= 10) //e.g. 3.30
		{
			version.major = major;
			version.minor = minor / 10;
		}
		else //e.g. 3.3
		{
			version.major = major;
			version.minor = minor;
		}
	}
	else
	{
		sendWarning("GlContext::init: invalid glsl version string: '", name, "' ", count);
		return version;
	}

	return version;
}

}

GlContext* GlContext::threadLocalCurrent(bool change, GlContext* newOne)
{
	static thread_local GlContext* current_ = nullptr;
	if(change)
		current_ = newOne;

	return current_;
}

void GlContext::assureGlLoaded(const GlContext& ctx)
{
	static bool loaded = false;
	if(!loaded)
	{
		gCtx = &ctx;
		gladLoadGLLoader(loadCallback);
		gCtx = nullptr;
		loaded = true;
	}
}

void GlContext::assureGlesLoaded(const GlContext& ctx)
{
	static bool loaded = false;
	if(!loaded)
	{
		gCtx = &ctx;
		gladLoadGLES2Loader(loadCallback);
		gCtx = nullptr;
		loaded = true;
	}
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

	//load the api function pointers via glad
	if(api_ == Api::gl) assureGlLoaded(*this);
	else if(api_ == Api::gles) assureGlesLoaded(*this);

	//version from glad
	majorVersion_ = GLVersion.major;
	minorVersion_ = GLVersion.minor;

	//extensions
	if(version() >= 30)
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
	if(api_ == Api::gl && version() >= 43)
	{
		auto number = 0;
		glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &number);

		for(auto i = 0; i < number; ++i)
		{
			std::string ver = (const char*) glGetStringi(GL_SHADING_LANGUAGE_VERSION, i);
			auto version = parseGlslVersion(ver);
			if(version.major != 0) glslVersions_.push_back(version);
		}
	}
	else
	{
		std::string ver = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
		auto version = parseGlslVersion(ver);
		if(version.major != 0) glslVersions_.push_back(version);
	}

	//TODO: choose highest if multiple are available
	if(glslVersions_.empty()) throw std::runtime_error("ny::GlContext: failed to get glsl version");
	else preferredGlslVersion_ = glslVersions_[0];

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

void GlContext::updateViewport(const Rect2f& viewport)
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
	//glFinish();
	return 1;
}

}
