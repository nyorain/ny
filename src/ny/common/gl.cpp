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
		using Error = GlContextErrorCode;
		auto error = static_cast<Error>(code);
		switch(error)
		{
			case Error::invalidConfig: return "Given config id is invalid";
			case Error::invalidSharedContext: return "Given share context is invalid/incompatible";
			case Error::invalidApi: return "Cannot create context with the given api value";
			case Error::invalidVersion: return "The given version is not a valid gl(es) version";

			case Error::contextAlreadyCurrent: return "GlContext was already current";
			case Error::contextAlreadyNotCurrent: return "GlContext was already not current";
			case Error::contextCurrentInAnotherThread: return "GlContext current in another thread";

			case Error::surfaceAlreadyCurrent: return "Given surface current in another thread";
			case Error::invalidSurface: return "Invalid surface object";
			case Error::incompatibleSurface: return "Incompatible surface object";

			case Error::extensionNotSupported: return "Required Extension not supported";

			default: return "<unkown error code>";
		}
	}
};

using GlCurrentMap = std::map<std::thread::id, std::pair<GlContext*, const GlSurface*>>;
GlCurrentMap& contextCurrentMap(std::mutex*& mutex)
{
	static GlCurrentMap smap;
	static std::mutex smutex;

	mutex = &smutex;
	return smap;
}

}

//gl version to stirng
std::string name(const GlVersion& v)
{
	auto ret = std::to_string(v.major) + "." + std::to_string(v.minor * 10);
	if(v.api == GlApi::gles) ret += " ES";
	return ret;
}

std::uintmax_t& glConfigNumber(GlConfigId& id)
	{ return reinterpret_cast<std::uintmax_t&>(id); }

GlConfigId& glConfigId(std::uintmax_t& number)
	{ return reinterpret_cast<GlConfigId&>(number); }

const std::uintmax_t& glConfigNumber(const GlConfigId& id)
	{ return reinterpret_cast<const std::uintmax_t&>(id); }

const GlConfigId& glConfigId(const std::uintmax_t& number)
	{ return reinterpret_cast<const GlConfigId&>(number); }

unsigned int rate(const GlConfig& config)
{
	//just some first ideas - feel free to edit/propose changes
	//depth or stencil values of 0 are better than a value of 344 or 13

	auto ret = 1u;

	if(config.depth == 32) ret += 20;
	else if(config.depth == 24) ret += 14;
	else if(config.depth == 16) ret += 5;
	else if(config.depth == 0) ret += 1;

	if(config.stencil == 8) ret += 10;
	else if(config.stencil == 16) ret += 3;
	else if(config.stencil == 0) ret += 1;

	if(config.samples == 0) ret += 5;
	else if(config.samples == 2) ret += 4;
	else if(config.samples == 4) ret += 3;
	else if(config.samples == 8) ret += 2;
	else if(config.samples == 16) ret += 1;

	if(config.red == config.green && config.red == config.blue && config.red == 8) ret += 20;
	if(config.red == config.green && config.red == config.blue && config.red == 16) ret += 5;
	else if(config.red + config.green + config.blue == 16) ret += 2;

	if(config.alpha == 8) ret += 12;
	else if(config.alpha == 1) ret += 1;

	if(config.doublebuffer) ret += 20;

	return ret;
}

std::error_code make_error_code(GlContextErrorCode code)
{
	return {(int) code, GlContextErrorCategory::instance()};
}

//GlContextError
GlContextError::GlContextError(GlContextErrorCode error, const char* msg)
	: GlContextError(std::error_code{error}, msg)
{
}

GlContextError::GlContextError(std::error_code code, const char* msg) : logic_error("")
{
	std::string whatMsg;
	if(msg) whatMsg.append(msg).append(": ");
	whatMsg.append(code.message());

	std::logic_error::operator=(std::logic_error(whatMsg));
}

//GlSetup
GlConfig GlSetup::config(GlConfigId id) const
{
	auto cfgs = configs();
	for(auto& cfg : cfgs) if(cfg.id == id) return cfg;
	return {};
}

//GlSurface
bool GlSurface::isCurrent(const GlContext** currentContext) const
{
	std::mutex* mutex;
	auto& map = contextCurrentMap(mutex);

	std::lock_guard<std::mutex> lock(*mutex);
	auto it = map.find(std::this_thread::get_id());
	if(it == map.end() || it->second.second != this) return false;

	if(currentContext) *currentContext = it->second.first;
	return true;
}

bool GlSurface::isCurrentInAnyThread(const GlContext** currentContext) const
{

	std::mutex* mutex;
	auto& map = contextCurrentMap(mutex);

	std::lock_guard<std::mutex> lock(*mutex);

	for(auto& entry : map)
	{
		if(entry.second.second == this)
		{
			if(currentContext) *currentContext = entry.second.first;
			return true;
		}
	}

	return false;
}

void GlSurface::apply() const
{
	std::error_code ec;
	if(!apply(ec)) throw std::system_error(ec, "ny::GlSurface::apply");
}

//GlContext - static
GlContext* GlContext::current(const GlSurface** currentSurface)
{
	std::mutex* mutex;
	auto& map = contextCurrentMap(mutex);

	std::lock_guard<std::mutex> lock(*mutex);
	auto it = map.find(std::this_thread::get_id());
	if(it == map.end()) return nullptr;

	if(currentSurface) *currentSurface = it->second.second;
	return it->second.first;
}

//GlContext
GlContext::~GlContext()
{
	//if current we ignore it and simply unregister it anyways
	//either the implementation is leaking or destroyed the context without making
	//it not current (in which case - if it did not raise an error - we can try to ignore it).
	if(isCurrent())
	{
		warning("ny::~GlContext: current on destruction. Implementation issue.");

		std::mutex* mutex;
		auto& map = contextCurrentMap(mutex);

		{
			std::lock_guard<std::mutex> lock(*mutex);
			map[std::this_thread::get_id()] = {nullptr, nullptr};
		}
	}
}

void GlContext::initContext(GlApi api, const GlConfig& config, const GlContext* shared)
{
	api_ = api;
	config_ = config;
	shared_ = shared;
}

void GlContext::makeCurrent(const GlSurface& surface)
{
	std::error_code error;
	if(!makeCurrent(surface, error))
	{
		if(&error.category() == &GlContextErrorCategory::instance())
			throw GlContextError(error, "ny::GlContext::makeCurrent");

		throw std::system_error(error, "ny::GlContext::makeCurrent");
	}
}

bool GlContext::makeCurrent(const GlSurface& surface, std::error_code& error)
{
	if(!surface.nativeHandle())
	{
		error = {ErrorCode::invalidSurface};
		return false;
	}

	if(!compatible(surface))
	{
		error = {ErrorCode::incompatibleSurface};
		return false;
	}

	//get the context current thread map
	std::mutex* mutex;
	auto threadid = std::this_thread::get_id();
	auto& map = contextCurrentMap(mutex);
	decltype(map.begin()) thisThreadIt {};
	error.clear();

	//prepare everything
	//check if this exact combination is already current
	{
		std::lock_guard<std::mutex> lock(*mutex);

		thisThreadIt = map.find(threadid);
		if(thisThreadIt == map.end()) thisThreadIt = map.insert({threadid, {}}).first;

		if(thisThreadIt->second.first == this && thisThreadIt->second.second == &surface)
		{
			error = {ErrorCode::contextAlreadyCurrent};
			return true; //return true since this is not critical
		}

		//check if this context or the given surface is already current in another
		//thread.
		for(auto& entry : map)
		{
			if(entry.first == threadid) continue;

			if(entry.second.first == this)
			{
				error = {ErrorCode::contextCurrentInAnotherThread};
				return false;
			}

			if(entry.second.second == &surface)
			{
				error = {ErrorCode::surfaceAlreadyCurrent};
				return false;
			}
		}

		//make current context not current and check for failure
		//dont just call makeNotCurrent here, since this would result in a deadlock
		if(thisThreadIt->second.first)
		{
			if(!thisThreadIt->second.first->makeNotCurrentImpl(error)) return false;
			thisThreadIt->second = {nullptr, nullptr};
		}
	}

	//makeCurrentImpl can be outside of an critical section
	//note how we still use thisThreadIt iterator after exiting critical section
	//map iterators are not invalidated  if they are not deleted (and this operation is
	//not done by any thread)
	if(makeCurrentImpl(surface, error))
	{
		std::lock_guard<std::mutex> lock(*mutex);
		thisThreadIt->second = {this, &surface};
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
		if(thisThreadIt == map.end()) thisThreadIt = map.insert({threadid, {}}).first;

		if(thisThreadIt->second.first != this)
		{
			error = {ErrorCode::contextAlreadyNotCurrent};
			return true;
		}
	}

	if(makeNotCurrentImpl(error))
	{
		std::lock_guard<std::mutex> lock(*mutex);
		thisThreadIt->second = {nullptr, nullptr};
		return true;
	}

	return false;
}

bool GlContext::isCurrent(const GlSurface** currentSurface) const
{
	std::mutex* mutex;
	auto& map = contextCurrentMap(mutex);

	std::lock_guard<std::mutex> lock(*mutex);
	auto it = map.find(std::this_thread::get_id());
	if(it == map.end() || it->second.first != this) return false;

	if(currentSurface) *currentSurface = it->second.second;
	return true;
}

bool GlContext::isCurrentInAnyThread(const GlSurface** currentSurface) const
{
	std::mutex* mutex;
	auto& map = contextCurrentMap(mutex);

	std::lock_guard<std::mutex> lock(*mutex);

	for(auto& entry : map)
	{
		if(entry.second.first == this)
		{
			if(currentSurface) *currentSurface = entry.second.second;
			return true;
		}
	}

	return false;
}

bool GlContext::sharedWith(const GlContext& other, bool pongBack) const
{
	//recursive; not trivial
	//note that if the context has no stored shared context is does only mean that it was
	//created without sharing but other contexts might have been created with sharing to
	//this context later on.

	//note: we do not explicitly check for compatibility here
	//should be no problem since this is checked on creation

	//brief:
	//Tries to find the other context in the single linked list of shared contexts
	//this context can access. If it does not find it (and still is in the first reursion,
	//i.e. pongBack == true), the last context checked asks the other context if itself
	//is part of the single linked list that the other context can access. But it sets
	//pongBack to false (i.e. signals it that it is now the 2. recursion).

	//first check whether this context has a shared stored context (the context it was
	//created shared with). If so check if it is the same as other.
	//If they both are not the same, other may still be "somewhere down the road".
	//The only condition where this recursion breaks, if the "end of the road" is reached,
	//i.e. the context that was created without any shared contexts is found.
	//If none of those contexts "along the road" were the other (searched for) context,
	//the last context (without any stored shared context) is the first one to
	//proceed to the next statement. Therefore pongBack is also passed down the road.
	if(shared() && (shared() == &other || shared()->sharedWith(other, pongBack))) return true;

	//So if other isnt one of the contexts it was created shared with, check whether this was
	//one of the contexts other was created shared with.
	//Important is to pass pongBack as false here, otherwise the recursion will infinitely
	//pong between 2 contexts.
	//So this call will simply trigger the other context to check all contexts it was created
	//shared with and if one of them is this context, true is returned.
	//This will only be called from the last context of the first recursion (i.e. the one
	//that was not created shared with any context) since if other was or was not created shared
	//with the last one, it is shared or not shared with all others in the recursion, and
	//therefore with the original one.
	if(!shared() && pongBack && other.sharedWith(*this, false)) return true;

	//Otherwise this context was not created shared with other and other was not created shared
	//with this, therefore the both contexts are not shared.
	return false;
}

bool GlContext::compatible(const GlSurface& surf) const
{
	return (config().id == surf.config().id);
}

void GlContext::swapInterval(int interval) const
{
	std::error_code ec;
	if(!swapInterval(interval, ec)) throw std::system_error(ec, "ny::GlContext::swapInterval");
}

bool GlContext::swapInterval(int interval, std::error_code& ec) const
{
	nytl::unused(interval);
	ec = {ErrorCode::extensionNotSupported};
	return false;
}

}
