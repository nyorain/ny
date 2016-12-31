// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/common/gl.hpp>
#include <ny/log.hpp>

#include <thread> // std::this_thread
#include <mutex> // std::mutex
#include <unordered_map> // std::unordered_map
#include <algorithm> // std::reverse

// This is a rather complex construct regarding synchronization, excpetion safety, sharing and
// making context/surface combinations current. Therefore it should only be altered if the
// developer knows the critical implementation parts.

namespace ny
{

namespace
{

//GlContext std::error_category implementation
class GlEC : public std::error_category
{
public:
	static GlEC& instance()
	{
		static GlEC ret;
		return ret;
	}

public:
	const char* name() const noexcept override { return "ny::GlContext"; }
	std::string message(int code) const override
	{
		using Error = GlContextErrc;
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

using GlCurrentMap = std::unordered_map<std::thread::id, std::pair<GlContext*, const GlSurface*>>;
GlCurrentMap& contextCurrentMap(std::mutex*& mutex)
{
	static GlCurrentMap smap;
	static std::mutex smutex;

	mutex = &smutex;
	return smap;
}

std::mutex& contextShareMutex()
{
	static std::mutex smutex;
	return smutex;
}

}

//gl version to stirng
std::string name(const GlVersion& v)
{
	auto ret = std::to_string(v.major) + "." + std::to_string(v.minor * 10);
	if(v.api == GlApi::gles) ret += " ES";
	return ret;
}

std::uintptr_t& glConfigNumber(GlConfigID& id)
	{ return reinterpret_cast<std::uintptr_t&>(id); }

GlConfigID& glConfigID(std::uintptr_t& number)
	{ return reinterpret_cast<GlConfigID&>(number); }

std::uintptr_t glConfigNumber(const GlConfigID& id)
	{ return reinterpret_cast<const std::uintmax_t&>(id); }

GlConfigID glConfigID(const std::uintmax_t& number)
	{ return reinterpret_cast<const GlConfigID&>(number); }

unsigned int rate(const GlConfig& config)
{
	//just some first ideas - feel free to edit/propose changes
	//depth or stencil values of 0 are better than a value of 344 or 13

	auto ret = 1u;

	if(config.depth == 24) ret += 20;
	else if(config.depth == 32) ret += 14;
	else if(config.depth == 16) ret += 5;
	else if(config.depth == 0) ret += 1;

	if(config.stencil == 8) ret += 10;
	else if(config.stencil == 16) ret += 3;
	else if(config.stencil == 0) ret += 1;

	if(config.samples == 0) ret += 10;
	else if(config.samples == 2) ret += 4;
	else if(config.samples == 4) ret += 3;
	else if(config.samples == 8) ret += 2;
	else if(config.samples == 16) ret += 1;

	if(config.red == config.green && config.red == config.blue && config.red == 8) ret += 20;
	if(config.red == config.green && config.red == config.blue && config.red == 16) ret += 5;
	else if(config.red + config.green + config.blue == 16) ret += 2;

	if(config.alpha == 8) ret = (ret + 5) * 2;
	else if(config.alpha == 1) ret += 1;

	if(config.doublebuffer) ret = (ret + 10) * 2;

	return ret;
}

std::error_condition make_error_condition(GlContextErrc code)
{
	return {static_cast<int>(code), GlEC::instance()};
}

std::error_code make_error_code(GlContextErrc code)
{
	return {static_cast<int>(code), GlEC::instance()};
}

//GlContextError
GlContextError::GlContextError(std::error_code code, const char* msg) : logic_error("")
{
	std::string whatMsg;
	if(msg) whatMsg.append(msg).append(": ");
	whatMsg.append(code.message());

	std::logic_error::operator=(std::logic_error(whatMsg));
}

//GlSetup
GlConfig GlSetup::config(GlConfigID id) const
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
		warning("ny::~GlContext: context is current on destruction");

		std::mutex* mutex;
		auto& map = contextCurrentMap(mutex);

		{
			std::lock_guard<std::mutex> lock(*mutex);
			map[std::this_thread::get_id()] = {nullptr, nullptr};
		}
	}

	{
		std::lock_guard<std::mutex> lock(contextShareMutex());
		for(auto& c : shared_)
		{
			if(!c->removeShared(*this))
				warning("ny::~GlContext: context->removeShared(*this) failed");
		}
	}
}

void GlContext::initContext(GlApi api, const GlConfig& config, GlContext* shared)
{
	api_ = api;
	config_ = config;

	if(shared)
	{
		std::lock_guard<std::mutex> lock(contextShareMutex());

		shared_ = shared->shared();
		shared_.push_back(shared);
		shared->addShared(*this);
	}
}

void GlContext::makeCurrent(const GlSurface& surface)
{
	std::error_code error;
	if(!makeCurrent(surface, error))
	{
		if(&error.category() == &GlEC::instance())
			throw GlContextError(error, "ny::GlContext::makeCurrent");

		throw std::system_error(error, "ny::GlContext::makeCurrent");
	}
}

bool GlContext::makeCurrent(const GlSurface& surface, std::error_code& ec)
{
	ec.clear();

	if(!surface.nativeHandle())
	{
		ec = Errc::invalidSurface;
		return false;
	}

	if(!compatible(surface))
	{
		ec = Errc::incompatibleSurface;
		return false;
	}

	//get the context current thread map
	std::mutex* mutex;
	auto threadid = std::this_thread::get_id();
	auto& map = contextCurrentMap(mutex);
	decltype(map.begin()) thisThreadIt {};
	ec.clear();

	//prepare everything
	//check if this exact combination is already current
	{
		std::lock_guard<std::mutex> lock(*mutex);

		thisThreadIt = map.find(threadid);
		if(thisThreadIt == map.end()) thisThreadIt = map.insert({threadid, {}}).first;

		if(thisThreadIt->second.first == this && thisThreadIt->second.second == &surface)
		{
			ec = Errc::contextAlreadyCurrent;
			return true; //return true since this is not critical, but we have nothing to do
		}

		//check if this context or the given surface is already current in another
		//thread.
		for(auto& entry : map)
		{
			if(entry.first == threadid) continue;

			if(entry.second.first == this)
			{
				ec = Errc::contextCurrentInAnotherThread;
				return false;
			}

			if(entry.second.second == &surface)
			{
				ec = Errc::surfaceAlreadyCurrent;
				return false;
			}
		}

		//make current context not current and check for failure
		//dont just call makeNotCurrent here, since this would result in a deadlock
		if(thisThreadIt->second.first)
		{
			if(!thisThreadIt->second.first->makeNotCurrentImpl(ec)) return false;
			thisThreadIt->second = {nullptr, nullptr};
		}
	}

	//makeCurrentImpl can be outside of an critical section
	//note how we still use thisThreadIt iterator after exiting critical section
	//map iterators are not invalidated  if they are not deleted (and this operation is
	//not done by any thread)
	if(!makeCurrentImpl(surface, ec)) return false;

	std::lock_guard<std::mutex> lock(*mutex);
	thisThreadIt->second = {this, &surface};
	return true;
}

void GlContext::makeNotCurrent()
{
	std::error_code error;
	if(!makeNotCurrent(error)) throw std::system_error(error);
}

bool GlContext::makeNotCurrent(std::error_code& ec)
{
	ec.clear();

	std::mutex* mutex;
	auto threadid = std::this_thread::get_id();
	auto& map = contextCurrentMap(mutex);
	decltype(map.begin()) thisThreadIt {};

	//check if it is already not current
	{
		std::lock_guard<std::mutex> lock(*mutex);

		thisThreadIt = map.find(threadid);
		if(thisThreadIt == map.end()) thisThreadIt = map.insert({threadid, {}}).first;

		if(thisThreadIt->second.first != this)
		{
			ec = Errc::contextAlreadyNotCurrent;
			return true;
		}
	}

	if(!makeNotCurrentImpl(ec)) return false;

	std::lock_guard<std::mutex> lock(*mutex);
	thisThreadIt->second = {nullptr, nullptr};
	return true;
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
	ec = Errc::extensionNotSupported;
	return false;
}

void GlContext::addShared(GlContext& other)
{
	shared_.push_back(&other);
}

bool GlContext::removeShared(GlContext& other)
{
	auto it = std::remove(shared_.begin(), shared_.end(), &other);
	if(it == shared_.end()) return false;

	shared_.erase(it, shared_.end());
	return true;
}

bool shared(GlContext& a, GlContext& b)
{
	std::lock_guard<std::mutex> lock(contextShareMutex());
	const auto& shared = a.shared();

	for(auto& c : shared) if(c == &b) return true;
	return false;
}

//GlCurrentGuard
GlCurrentGuard::GlCurrentGuard(GlContext& ctx, const GlSurface& surface) : context(ctx)
{
	context.makeCurrent(surface);
}

GlCurrentGuard::~GlCurrentGuard()
{
	try{ context.makeNotCurrent(); }
	catch(const std::exception& exception) { warning("ny::~GlCurrentGuard: ", exception.what()); }
}

}
