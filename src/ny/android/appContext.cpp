// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/appContext.hpp>
#include <ny/android/windowContext.hpp>
#include <ny/log.hpp>
#include <ny/loopControl.hpp>

#ifdef NY_WithVulkan
 #define VK_USE_PLATFORM_ANDROID_KHR
 #include <ny/android/vulkan.hpp>
 #include <vulkan/vulkan.h>
#endif // Vulkan

#ifdef NY_WithEgl
 #include <ny/common/egl.hpp>
 #include <ny/android/egl.hpp>
#endif // Egl

#include <queue>
#include <functional>

namespace ny {
namespace {

/// Android LoopInterface implementation
/// Just uses the ALooper mechnisms
class AndroidLoopImpl : public ny::LoopInterface {
public:
	ALooper& looper;
	std::atomic<bool> run {true};
	std::queue<std::function<void()>> functions {};
	std::mutex mutex {};

public:
	AndroidLoopImpl(LoopControl& lc, ALooper& alooper)
		: LoopInterface(lc), looper(alooper)
	{
	}

	bool stop() override
	{
		run.store(false);
		wakeup();
		return true;
	}

	bool call(std::function<void()> function) override
	{
		if(!function) return false;

		{
			std::lock_guard<std::mutex> lock(mutex);
			functions.push(std::move(function));
		}

		wakeup();
		return true;
	}

	// Wake the looper up
	void wakeup()
	{
		ALooper_wake(&looper);
	}

	// Returns the first queued function to be called or an empty object.
	std::function<void()> popFunction()
	{
		std::lock_guard<std::mutex> lock(mutex);
		if(functions.empty()) return {};
		auto ret = std::move(functions.front());
		functions.pop();
		return ret;
	}
};

} // anonymous util namespace

struct AndroidAppContext::Impl {
#ifdef NY_WithEgl
	bool eglFailed;
	EglSetup eglSetup;
#endif //WithEGL
};

// AndroidAppContext
AndroidAppContext::AndroidAppContext(android::Activity& activity) : activity_(activity)
{
	impl_ = std::make_unique<Impl>();
	looper_ = ALooper_prepare(0);
}

AndroidAppContext::~AndroidAppContext()
{

}

WindowContextPtr AndroidAppContext::createWindowContext(const WindowSettings& settings)
{
	const static std::string func = "ny::AndroidAppContext::createWindowContext: ";
	if(!activity_.nativeWindow())
		throw std::runtime_error(func + " no native android window retrieved");

	if(windowContext_)
		throw std::logic_error(func + " android can only create one windowContext");

	AndroidWindowSettings androidSettings;
	const auto* ws = dynamic_cast<const AndroidWindowSettings*>(&settings);

	if(ws) androidSettings = *ws;
	else androidSettings.WindowSettings::operator=(settings);

	return std::make_unique<AndroidWindowContext>(*this, androidSettings);
}

bool AndroidAppContext::dispatchEvents()
{
	int outFd, outEvents;
	void* outData;
	auto ret = ALooper_pollOnce(0, &outFd, &outEvents, &outData);
	if(ret == ALOOPER_POLL_ERROR)
		warning("ny::AndroidAppContext::dispatchLoop: ALooper_pollAll returned error");

	return true;
}

bool AndroidAppContext::dispatchLoop(LoopControl& loopControl)
{
	AndroidLoopImpl loopImpl(loopControl, *looper_);

	while(loopImpl.run.load()) {
		int outFd, outEvents;
		void* outData;
		auto ret = ALooper_pollAll(-1, &outFd, &outEvents, &outData);
		if(ret == ALOOPER_POLL_ERROR)
			warning("ny::AndroidAppContext::dispatchLoop: ALooper_pollAll returned error");
	}

	return true;
}

std::vector<const char*> AndroidAppContext::vulkanExtensions() const
{
	#ifdef NY_WithVulkan
		return {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
	#else
		return {};
	#endif
}

GlSetup* AndroidAppContext::glSetup() const
{
	#ifdef NY_WithEgl
		return eglSetup();
	#else
		return nullptr;
	#endif // WithEgl
}

EglSetup* AndroidAppContext::eglSetup() const
{
	#ifdef NY_WithEgl
		if(impl_->eglFailed) return nullptr;

		if(!impl_->eglSetup.valid()) {
			try { impl_->eglSetup = {nullptr}; }
			catch(const std::exception& error) {
				warning("WaylandAppContext::eglSetup: creating failed: ", error.what());
				impl_->eglFailed = true;
				impl_->eglFailed = {};
				return nullptr;
			}
		}

		return &impl_->eglSetup;

	#else
		return nullptr;
	#endif
}

} // namespace ny
