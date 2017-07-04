// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/common/gl.hpp>
#include <ny/log.hpp>

#include <thread> // std::this_thread
#include <mutex> // std::mutex
#include <unordered_map> // std::unordered_map
#include <algorithm> // std::reverse
#include <cstring> // std::strstr

// This is a rather complex construct regarding synchronization, excpetion safety, sharing and
// making context/surface combinations current. Therefore it should only be altered if the
// developer knows the critical implementation parts.

namespace ny {
namespace {

/// GlContext std::error_category implementation
/// Used to provide abstract logical gl error codes.
/// Implemented at the bottom of this file.
class GlErrorCategory : public std::error_category {
public:
	static GlErrorCategory& instance();

public:
	const char* name() const noexcept override { return "ny::GlErrorCategory"; }
	std::string message(int code) const override;
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

} // anonymous util namespace

// gl version to stirng
std::uintptr_t& glConfigNumber(GlConfigID& id)
{
	return reinterpret_cast<std::uintptr_t&>(id);
}

GlConfigID& glConfigID(std::uintptr_t& number)
{
	return reinterpret_cast<GlConfigID&>(number);
}

std::uintptr_t glConfigNumber(const GlConfigID& id)
{
	return reinterpret_cast<const std::uintmax_t&>(id);
}

GlConfigID glConfigID(const std::uintptr_t& number)
{
	return reinterpret_cast<const GlConfigID&>(number);
}

unsigned int rate(const GlConfig& config)
{
	// just some first ideas - feel free to edit/propose changes
	// remember depth or stencil values of 0 are better than e.g. a value of 344 or -42
	// that is why even 0 gets some rating points

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

bool glExtensionStringContains(nytl::StringParam extString, nytl::StringParam extension)
{
	auto it = extString.data();
	while(true) {
		auto loc = std::strstr(it, extension);
		if(!loc)
			return false;

		auto terminator = loc + std::strlen(extension);
		bool blankBefore = (loc == extString || *(loc - 1) == ' ');
		bool blankAfter = (*terminator == ' ' || *terminator == '\0');
		if(blankBefore && blankAfter)
			return true;

		it = terminator;
	}

	return false;
}

std::error_condition make_error_condition(GlContextErrc code)
{
	return {static_cast<int>(code), GlErrorCategory::instance()};
}

std::error_code make_error_code(GlContextErrc code)
{
	return {static_cast<int>(code), GlErrorCategory::instance()};
}

// GlContextError
GlContextError::GlContextError(std::error_code code, nytl::StringParam msg) : logic_error("")
{
	std::string whatMsg;
	if(msg) whatMsg.append(msg).append(": ");
	whatMsg.append(code.message());

	std::logic_error::operator=(std::logic_error(whatMsg));
}

// GlSetup
GlConfig GlSetup::config(GlConfigID id) const
{
	auto cfgs = configs();
	for(auto& cfg : cfgs)
		if(cfg.id == id)
			return cfg;

	return {};
}

std::unique_ptr<GlContext> GlSetup::createContext(const GlSurface& surface,
	GlContextSettings settings) const
{
	settings.config = surface.config().id;
	return createContext(settings);
}

// GlSurface
GlSurface::~GlSurface()
{
	// same as ~GlContext:
	// we ignore it and simply unregister it anyways
	// either the implementation is leaking or destroyed the context without making
	// it not current (in which case - if it did not raise an error - we can try to ignore it).
	GlContext* context {};
	if(isCurrent(&context)) {
		ny_warn("~GlSurface"_scope, "current in calling thread!");

		std::mutex* mutex;
		auto& map = contextCurrentMap(mutex);

		{
			std::lock_guard<std::mutex> lock(*mutex);
			map[std::this_thread::get_id()] = {nullptr, nullptr};
		}
	}

	// check if it is current in any thread
	// we cannot use isCurrentInAnyThread since the checking/removing has
	// to be in one critical section, it has to be atomic (regarding the current map mutex)
	std::mutex* mutex;
	auto& map = contextCurrentMap(mutex);

	std::lock_guard<std::mutex> lock(*mutex);

	for(auto& entry : map) {
		if(entry.second.second == this) {
			ny_error("~GlSurface"_scope, "current in another thread");
			entry.second = {nullptr, nullptr};
		}
	}
}

bool GlSurface::isCurrent(GlContext** currentContext) const
{
	std::mutex* mutex;
	auto& map = contextCurrentMap(mutex);

	std::lock_guard<std::mutex> lock(*mutex);
	auto it = map.find(std::this_thread::get_id());
	if(it == map.end() || it->second.second != this) return false;

	if(currentContext) *currentContext = it->second.first;
	return true;
}

bool GlSurface::isCurrentInAnyThread(GlContext** currentContext,
	std::thread::id* currentThread) const
{

	std::mutex* mutex;
	auto& map = contextCurrentMap(mutex);

	std::lock_guard<std::mutex> lock(*mutex);

	for(auto& entry : map) {
		if(entry.second.second == this) {
			if(currentContext) *currentContext = entry.second.first;
			if(currentThread) *currentThread = entry.first;
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

// GlContext - static
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

// GlContext
GlContext::~GlContext()
{
	// we ignore it and simply unregister it anyways
	// either the implementation is leaking or destroyed the context without making
	// it not current (in which case - if it did not raise an error - we can try to ignore it).
	if(isCurrent()) {
		ny_warn("~GlContext"_scope, "context is current on destruction");

		std::mutex* mutex;
		auto& map = contextCurrentMap(mutex);

		{
			std::lock_guard<std::mutex> lock(*mutex);
			map[std::this_thread::get_id()] = {nullptr, nullptr};
		}
	}

	// check if it is current in any thread
	// we cannot use isCurrentInAnyThread since the checking/removing has
	// to be in one critical section, it has to be atomic (regarding the current map mutex)
	{
		std::mutex* mutex;
		auto& map = contextCurrentMap(mutex);

		std::lock_guard<std::mutex> lock(*mutex);

		for(auto& entry : map) {
			if(entry.second.first == this) {
				ny_error("~GlContext"_scope, "current in another thread");
				entry.second = {nullptr, nullptr};
			}
		}
	}

	// signal the shared contexts
	{
		std::lock_guard<std::mutex> lock(contextShareMutex());
		for(auto& c : shared_) {
			if(!c->removeShared(*this))
				ny_warn("~GlContext"_scope, "context->removeShared(*this) failed - expect data inconsistency");
		}
	}
}

void GlContext::initContext(GlApi api, const GlConfig& config, GlContext* shared)
{
	api_ = api;
	config_ = config;

	if(shared) {
		std::lock_guard<std::mutex> lock(contextShareMutex());

		shared_ = shared->shared();
		shared_.push_back(shared);

		for(auto& s : shared_)
			s->addShared(*this);
	}
}

void GlContext::makeCurrent(const GlSurface& surface)
{
	std::error_code error;
	if(!makeCurrent(surface, error)) {
		if(&error.category() == &GlErrorCategory::instance())
			throw GlContextError(error, "ny::GlContext::makeCurrent");

		throw std::system_error(error, "ny::GlContext::makeCurrent");
	}
}

bool GlContext::makeCurrent(const GlSurface& surface, std::error_code& ec)
{
	ec.clear();

	if(!surface.nativeHandle()) {
		ec = Errc::invalidSurface;
		return false;
	}

	if(!compatible(surface)) {
		ec = Errc::incompatibleSurface;
		return false;
	}

	// get the context current thread map
	std::mutex* mutex;
	auto threadid = std::this_thread::get_id();
	auto& map = contextCurrentMap(mutex);
	decltype(map.begin()) thisThreadIt {};
	ec.clear();

	// prepare everything
	// check if this exact combination is already current
	{
		std::lock_guard<std::mutex> lock(*mutex);

		thisThreadIt = map.find(threadid);
		if(thisThreadIt == map.end()) thisThreadIt = map.insert({threadid, {}}).first;

		if(thisThreadIt->second.first == this && thisThreadIt->second.second == &surface) {
			ec = Errc::contextAlreadyCurrent;
			return true; // return true since this is not critical, but we have nothing to do
		}

		// check if this context or the given surface is already current in another
		// thread.
		for(auto& entry : map) {
			if(entry.first == threadid) continue;

			if(entry.second.first == this) {
				ec = Errc::contextCurrentInAnotherThread;
				return false;
			}

			if(entry.second.second == &surface) {
				ec = Errc::surfaceAlreadyCurrent;
				return false;
			}
		}

		// make current context not current and check for failure
		// dont just call makeNotCurrent here, since this would result in a deadlock
		if(thisThreadIt->second.first) {
			if(!thisThreadIt->second.first->makeNotCurrentImpl(ec)) return false;
			thisThreadIt->second = {nullptr, nullptr};
		}
	}

	// makeCurrentImpl can be outside of an critical section
	// note how we still use thisThreadIt iterator after exiting critical section
	// map iterators are not invalidated  if they are not deleted (and this operation is
	// not done by any thread)
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

	// check if it is already not current
	{
		std::lock_guard<std::mutex> lock(*mutex);

		thisThreadIt = map.find(threadid);
		if(thisThreadIt == map.end()) thisThreadIt = map.insert({threadid, {}}).first;

		if(thisThreadIt->second.first != this) {
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

bool GlContext::isCurrentInAnyThread(const GlSurface** currentSurface,
	std::thread::id* currentThread) const
{
	std::mutex* mutex;
	auto& map = contextCurrentMap(mutex);

	std::lock_guard<std::mutex> lock(*mutex);

	for(auto& entry : map) {
		if(entry.second.first == this) {
			if(currentSurface) *currentSurface = entry.second.second;
			if(currentThread) *currentThread = entry.first;
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

// GlCurrentGuard
GlCurrentGuard::GlCurrentGuard(GlContext& ctx, const GlSurface& surface) : context(ctx)
{
	context.makeCurrent(surface);
}

GlCurrentGuard::~GlCurrentGuard()
{
	try {
		context.makeNotCurrent();
	} catch(const std::exception& exception) {
		ny_warn("~GlCurrentGuard"_scope, "makeNotCurrent failed: {}", exception.what());
	}
}

// GlErrorCategory implementation
GlErrorCategory& GlErrorCategory::instance()
{
	static GlErrorCategory ret;
	return ret;
}

std::string GlErrorCategory::message(int code) const
{
	using Error = GlContextErrc;
	auto error = static_cast<Error>(code);

	switch(error) {
		case Error::invalidConfig: return "Given config id is invalid";
		case Error::invalidSharedContext: return "Given share context is invalid/incompatible";
		case Error::invalidApi: return "Cannot create context with the given api value";
		case Error::invalidVersion: return "The given api version is invalid";

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

} // namespace ny
