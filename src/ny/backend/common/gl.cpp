#include <ny/backend/common/gl.hpp>
#include <ny/base/log.hpp>
#include <nytl/misc.hpp>

#include <thread>

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
GlContext::Version parseGlslVersion(const std::string& name)
{
	GlContext::Version version;
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

//gl version to stirng
std::string GlContext::Version::name() const
{
	auto ret = std::to_string(major) + "." + std::to_string(minor * 10);
	if(api == GlContext::Api::gles) ret += " ES";
	return ret;
}


//GlContext
GlContext* GlContext::threadLocalCurrent(bool change, GlContext* newOne)
{
	static thread_local GlContext* current_ = nullptr;
	if(change) current_ = newOne;
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

//non-static
GlContext::~GlContext()
{
	makeNotCurrent();
}

void GlContext::initContext(Api api, unsigned int depth, unsigned int stencil)
{
	version_.api = api;
	depthBits_ = depth;
	stencilBits_ = stencil;

	auto* saved = current();

	//some backends need to make it current before
	if(saved == this) saved = nullptr;
	else if(!makeCurrent()) throw std::runtime_error("GlCtx::initCtx: failed to make current");

	//load the api function pointers via glad
	if(api == Api::gl) assureGlLoaded(*this);
	else if(api == Api::gles) assureGlesLoaded(*this);

	//version from glad
	version_.major = GLVersion.major;
	version_.minor = GLVersion.minor;

	//extensions
	if(versionNumber() >= 30)
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
	if(api == Api::gl && versionNumber() >= 43)
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

	if(glslVersions_.empty()) throw std::runtime_error("ny::GlContext: failed to get glsl version");

	//choose highest version
	auto& pgv = preferredGlslVersion_;
	for(auto& glsl : glslVersions_)	if(glsl.number() > pgv.number()) pgv = glsl;

	//make it not current - needed since it might be made current in another thread
	makeNotCurrent();

	//restore saved one if there is any
	if(saved && !saved->makeCurrent()) sendWarning("GlCtx::initCtx: failed to make saved current.");
}

bool GlContext::makeCurrent()
{
	if(isCurrent()) return true;

	if(makeCurrentImpl())
	{
		threadLocalCurrent(true, this);
		return true;
	}

	return false;
}

bool GlContext::makeNotCurrent()
{
	if(!isCurrent()) return true;

	//TODO: if branch like in makeCurrent?
	threadLocalCurrent(true, nullptr);
	return makeNotCurrentImpl();
}

bool GlContext::isCurrent() const
{
	return (threadLocalCurrent() == this);
}

bool GlContext::sharedWith(const GlContext& other) const
{
	for(auto& ctx : sharedContexts())
		if(ctx == &other) return true;

	return false;
}

bool GlContext::glExtensionSupported(const std::string& name) const
{
	for(auto& s : glExtensions())
		if(s == name) return true;

	return false;
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
	return true;
}

}
