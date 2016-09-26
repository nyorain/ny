#include <ny/backend/common/gl.hpp>
#include <ny/base/log.hpp>
#include <nytl/misc.hpp>

#include <thread>
#include <cstring>

namespace ny
{


namespace
{

//the needed parts of the opengl spec
using PfnGetString = const char*(*)(unsigned int);
using PfnGetStringi = const char*(*)(unsigned int, unsigned int);
using PfnGetIntegerv = void*(*)(unsigned int, int*);

constexpr unsigned int GL_EXTENSIONS = 0x1F03;
constexpr unsigned int GL_NUM_EXTENSIONS = 0x821D;
constexpr unsigned int GL_NUM_SHADING_LANGUAGE_VERSIONS = 0x82E9;
constexpr unsigned int GL_SHADING_LANGUAGE_VERSION = 0x8B8C;
constexpr unsigned int GL_MAJOR_VERSION = 0x821B;
constexpr unsigned int GL_MINOR_VERSION = 0x821C;
constexpr unsigned int GL_VERSION = 0x1F02;

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
		warning("GlContext::init: invalid glsl version string: '", name);
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
		warning("GlContext::init: invalid glsl version string: '", name, "' ", count);
		return version;
	}

	return version;
}

}

//gl version to stirng
std::string name(const GlVersion& v)
{
	auto ret = std::to_string(v.major) + "." + std::to_string(v.minor * 10);
	if(v.api == GlContext::Api::gles) ret += " ES";
	return ret;
}


//GlContext
GlContext* GlContext::threadLocalCurrent(bool change, GlContext* newOne)
{
	static thread_local GlContext* current_ = nullptr;
	if(change) current_ = newOne;
	return current_;
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
	else if(!makeCurrent())
		throw std::runtime_error("ny::GlContext::initContext: failed to make current");

	//load needed functions
	auto getString = reinterpret_cast<PfnGetString>(procAddr("glGetString"));
	auto getStringi = reinterpret_cast<PfnGetStringi>(procAddr("glGetStringi"));
	auto getIntegerv = reinterpret_cast<PfnGetIntegerv>(procAddr("glGetIntegerv"));

	if(!getString || !getStringi || !getIntegerv)
		throw std::runtime_error("ny::GlContext::initContext: failed to load gl functions.");

	//get opengl version
	//first try >3.0 api
	int major = 0, minor = 0;
	getIntegerv(GL_MAJOR_VERSION, &major);
	getIntegerv(GL_MINOR_VERSION, &minor);

	if(!major)
	{
		auto version = getString(GL_VERSION);
		if(!version)
			throw std::runtime_error("ny::GlContext::initContext: Unable to retrieve gl version");

		//version prefixes used by some implementers
		constexpr const char* prefixes[] = {
	        "OpenGL ES-CM ",
	        "OpenGL ES-CL ",
	        "OpenGL ES "
    	};

    	for (auto i = 0u; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i)
		{
        	const auto length = std::strlen(prefixes[i]);
        	if(strncmp(version, prefixes[i], length) == 0)
			{
            	version += length;
            	break;
        	}
		}

		std::sscanf(version, "%d.%d", &major, &minor);
 	}

	version_ = {api, static_cast<unsigned int>(major), static_cast<unsigned int>(minor)};

	//extensions
	if(versionNumber() >= 30)
	{
		auto number = 0;
		getIntegerv(GL_NUM_EXTENSIONS, &number);

		for(auto i = 0; i < number; ++i)
		{
			std::string ext = getStringi(GL_EXTENSIONS, i);
			extensions_.push_back(ext);
		}
	}
	else
	{
		std::string ext = getString(GL_EXTENSIONS);
		extensions_ = nytl::split(ext, ' ');
	}

	//glsl
	if(api == Api::gl && versionNumber() >= 43)
	{
		auto number = 0;
		getIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &number);

		for(auto i = 0; i < number; ++i)
		{
			std::string ver = getStringi(GL_SHADING_LANGUAGE_VERSION, i);
			auto version = parseGlslVersion(ver);
			if(version.major != 0) glslVersions_.push_back(version);
		}
	}
	else
	{
		std::string ver = getString(GL_SHADING_LANGUAGE_VERSION);
		auto version = parseGlslVersion(ver);
		if(version.major != 0) glslVersions_.push_back(version);
	}

	if(glslVersions_.empty()) throw std::runtime_error("ny::GlContext: failed to get glsl version");

	//choose highest version
	auto& pgv = preferredGlslVersion_;
	for(auto& glsl : glslVersions_)	if(number(glsl) > number(pgv)) pgv = glsl;

	//make it not current - needed since it might be made current in another thread
	makeNotCurrent();

	//restore saved one if there is any
	if(saved && !saved->makeCurrent()) warning("GlCtx::initCtx: failed to make saved current.");
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

bool GlContext::apply()
{
	return true;
}

}
