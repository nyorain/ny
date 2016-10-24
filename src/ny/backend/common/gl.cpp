#include <ny/backend/common/gl.hpp>
#include <ny/base/log.hpp>
#include <nytl/misc.hpp>

#include <thread>
#include <cstring>
#include <mutex>
#include <map>

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

//GlContext std::error_category implementation
class GlContextErrorCategory : public std::error_category
{
public:
	static GlContextErrorCategory& instance()
	{
		static GlContextErrorCategory ret;
		return ret;
	}

public:
	const char* name() const noexcept override { return "ny::GlContextErrorGategory"; }
	std::string message(int code) const override
	{
		using Error = GlContext::Error;
		auto error = static_cast<Error>(code);
		switch(error)
		{
			case Error::alreadyCurrent: return "GlContext was already current";
			case Error::alreadyNotCurrent: return "GlContext was already not current";
			case Error::currentInAnotherThread: return "The native gl context handle is already "
				" current in another thread.";
			default: return "<unkown error code>";
		}
	}
};

//parsing shader version
GlContext::Version parseGlslVersion(const std::string& name)
{
	GlContext::Version version;
	version.api = GlContext::Api::gl;

	auto pos = name.find(" es ");
	if(pos == std::string::npos) pos = name.find(" ES");
	if(pos == std::string::npos) pos = name.find("-es");
	if(pos == std::string::npos) pos = name.find("-es");
	if(pos == std::string::npos) pos = name.find("-ES");

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
		warning("GlContext::init: invalid glsl version string: '", name, "' c=", count);
		return version;
	}

	return version;
}

std::map<std::thread::id, GlContext*>& contextCurrentMap(std::mutex*& mutex)
{
	static std::map<std::thread::id, GlContext*> smap;
	static std::mutex smutex;

	mutex = &smutex;
	return smap;
}

}

//gl version to stirng
std::string name(const GlVersion& v)
{
	auto ret = std::to_string(v.major) + "." + std::to_string(v.minor * 10);
	if(v.api == GlContext::Api::gles) ret += " ES";
	return ret;
}

GlContext* GlContext::current()
{
	std::mutex* mutex;
	auto& map = contextCurrentMap(mutex);

	std::lock_guard<std::mutex> lock(*mutex);
	auto it = map.find(std::this_thread::get_id());
	if(it == map.end()) return nullptr;
	return it->second;
}

//GlContext
GlContext::~GlContext()
{
	//if current we ignore it and simply unregister it anyways
	if(isCurrent())
	{
		warning("ny::~GlContext: current on destruction. Implementation issue.");

		std::mutex* mutex;
		auto& map = contextCurrentMap(mutex);

		{
			std::lock_guard<std::mutex> lock(*mutex);
			map[std::this_thread::get_id()] = this;
		}
	}
}

void GlContext::initContext(Api api, unsigned int depth, unsigned int stencil)
{
	std::error_code error;
	version_.api = api;
	depthBits_ = depth;
	stencilBits_ = stencil;

	auto* saved = current();

	//some backends need to make it current before
	if(saved == this) saved = nullptr;
	else if(!makeCurrent(error))
		throw std::system_error(error, "ny::GlContext::initContext: failed to make current.");

	//load needed functions
	auto getString = reinterpret_cast<PfnGetString>(procAddr("glGetString"));
	auto getStringi = reinterpret_cast<PfnGetStringi>(procAddr("glGetStringi"));
	auto getIntegerv = reinterpret_cast<PfnGetIntegerv>(procAddr("glGetIntegerv"));

	if(!getString || !getStringi || !getIntegerv)
		throw std::runtime_error("ny::GlContext::initContext: failed to load gl functions.");

	//get opengl version
	//first try >3.0 api. Implementation issue.
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

		for(auto& prefix : prefixes)
		{
			const auto length = std::strlen(prefix);
			if(strncmp(version, prefix, length) == 0)
			{
				if(api != Api::gles)
					warning("ny::GlContext::initContext: ES version string for gl context");

				version += length;
				break;
			}
		}

		std::sscanf(version, "%d.%d", &major, &minor);
	 }

	version_ = {api, static_cast<unsigned int>(major), static_cast<unsigned int>(minor)};

	//extensions
	if(number(version()) >= 30)
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
	if(api == Api::gl && number(version()) >= 43)
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
	if(!makeNotCurrent(error))
		warning("ny::GlContext::initContext: failed to make context not current at the end.");

	//restore saved one if there is any
	if(saved && !saved->makeCurrent(error))
		warning("ny::GlContext::initContext: failed to make saved current: ", error.message());
}

void GlContext::makeCurrent()
{
	std::error_code error;
	if(!makeCurrent(error)) throw std::system_error(error);
}

bool GlContext::makeCurrent(std::error_code& error)
{
	std::mutex* mutex;
	auto threadid = std::this_thread::get_id();
	auto& map = contextCurrentMap(mutex);
	decltype(map.begin()) thisThreadIt {};
	error.clear();

	{
		std::lock_guard<std::mutex> lock(*mutex);

		thisThreadIt = map.find(threadid);
		if(thisThreadIt == map.end()) thisThreadIt = map.emplace(threadid, nullptr).first;

		//check if already current
		if(thisThreadIt->second == this)
		{
			auto code = static_cast<int>(Error::alreadyCurrent);
			error = std::error_code(code, GlContextErrorCategory::instance());
			return true;
		}

		//check if another context with same handle is already current
		for(auto& entry : map)
		{
			if(entry.second && entry.second->sameContext(*this))
			{
				auto code = static_cast<int>(Error::currentInAnotherThread);
				error = std::error_code(code, GlContextErrorCategory::instance());
				return false;
			}
		}

		//make current context not current and check for failure
		if(thisThreadIt->second)
		{
			if(!thisThreadIt->second->makeNotCurrent(error))
				return false;
		}
	}

	if(makeCurrentImpl(error))
	{
		std::lock_guard<std::mutex> lock(*mutex);
		thisThreadIt->second = this;
		return true;
	}

	return false;
}

void GlContext::makeNotCurrent()
{
	std::error_code error;
	if(!makeNotCurrent(error)) throw std::system_error(error);
}

bool GlContext::makeNotCurrent(std::error_code& error)
{
	std::mutex* mutex;
	auto threadid = std::this_thread::get_id();
	auto& map = contextCurrentMap(mutex);
	decltype(map.begin()) thisThreadIt {};
	error.clear();

	//check if it is already not current
	{
		std::lock_guard<std::mutex> lock(*mutex);

		thisThreadIt = map.find(threadid);
		if(thisThreadIt == map.end()) thisThreadIt = map.emplace(threadid, nullptr).first;
		if(thisThreadIt->second != this)
		{
			auto code = static_cast<int>(Error::alreadyNotCurrent);
			error = std::error_code(code, GlContextErrorCategory::instance());
			return true;
		}
	}

	if(makeNotCurrentImpl(error))
	{
		std::lock_guard<std::mutex> lock(*mutex);
		thisThreadIt->second = nullptr;
		return true;
	}

	return false;
}

bool GlContext::isCurrent() const
{
	std::mutex* mutex;
	auto& map = contextCurrentMap(mutex);

	std::lock_guard<std::mutex> lock(*mutex);
	return map[std::this_thread::get_id()] == this;
}

bool GlContext::currentAnywhere() const
{
	std::mutex* mutex;
	auto& map = contextCurrentMap(mutex);

	{
		std::lock_guard<std::mutex> lock(*mutex);

		for(auto& entry : map)
		{
			if(entry.second == this) return true;
		}
	}

	return false;
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

void GlContext::apply()
{
	std::error_code error;
	if(!apply(error)) throw std::system_error(error);
}

bool GlContext::sameContext(const GlContext& other) const
{
	return (nativeHandle() == other.nativeHandle()) && (nativeHandle() != nullptr);
}

}
