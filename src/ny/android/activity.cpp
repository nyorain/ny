// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/activity.hpp>
#include <ny/android/appContext.hpp>
#include <ny/log.hpp>
#include <nytl/tmpUtil.hpp>

#include <android/log.h>
#include <cstring> // std::memcpy

// NOTE: we will never throw from inside the callbacks
// since this might end badly considering android general
// poor c++ exception support
// Logging the message is the best we can do

// External main declaration
// implemented by application
extern int main(int, char**);

namespace ny::android {

// custom logger implementation to use the android log
class AndroidLogger : public LoggerBase {
public:
	AndroidLogger(int prio, const char* tag) : prio_(prio), tag_(tag) {}

	void write(const std::string& text) const override {
		__android_log_write(prio_, tag_, text.c_str());
	}

protected:
	int prio_;
	const char* tag_;
};

// Activity - static
Activity* Activity::instance()
{
	auto& singleton = instanceUnchecked();
	if(singleton.valid()) return &singleton;
	return nullptr;
}

Activity& Activity::instanceUnchecked()
{
	static Activity singleton;
	return singleton;
}

void Activity::onStart(ANativeActivity* nativeActivity) noexcept
{
	auto& activity = retrieveActivity(*nativeActivity);
	std::lock_guard<std::mutex> lock(activity.mutex_);
	auto appContext = activity.appContext_;

	if(appContext)
		appContext->pushEvent({ActivityEventType::start});
}
void Activity::onResume(ANativeActivity* nativeActivity) noexcept
{
	auto& activity = retrieveActivity(*nativeActivity);
	std::lock_guard<std::mutex> lock(activity.mutex_);
	auto appContext = activity.appContext_;

	if(appContext)
		appContext->pushEvent({ActivityEventType::resume});
}
void Activity::onPause(ANativeActivity* nativeActivity) noexcept
{
	auto& activity = retrieveActivity(*nativeActivity);
	std::lock_guard<std::mutex> lock(activity.mutex_);
	auto appContext = activity.appContext_;

	if(appContext)
		appContext->pushEvent({ActivityEventType::pause});
}
void Activity::onStop(ANativeActivity* nativeActivity) noexcept
{
	auto& activity = retrieveActivity(*nativeActivity);
	std::lock_guard<std::mutex> lock(activity.mutex_);
	auto appContext = activity.appContext_;

	if(appContext)
		appContext->pushEvent({ActivityEventType::stop});
}
void Activity::onDestroy(ANativeActivity* nativeActivity) noexcept
{
	// TODO: may not work as intended

	auto& activity = retrieveActivity(*nativeActivity);
	std::lock_guard<std::mutex> lock(activity.mutex_);
	activity.destroy();
	if(activity.appContext_)
		activity.appContext_->pushEventWait({ActivityEventType::destroy});
}
void Activity::onWindowFocusChanged(ANativeActivity* nativeActivity, int hasFocus) noexcept
{
	auto& activity = retrieveActivity(*nativeActivity);
	std::lock_guard<std::mutex> lock(activity.mutex_);
	if(activity.appContext_) {
		ActivityEvent ae;
		ae.type = ActivityEventType::windowFocus;
		ae.data[0] = hasFocus;
		activity.appContext_->pushEvent(ae);
	}
}
void Activity::onNativeWindowCreated(ANativeActivity* nativeActivity,
	ANativeWindow* window) noexcept
{
	auto& activity = retrieveActivity(*nativeActivity);
	std::lock_guard<std::mutex> lock(activity.mutex_);
	activity.window_ = window;
	if(activity.appContext_) {
		ActivityEvent ae;
		ae.type = ActivityEventType::windowCreated;
		std::memcpy(ae.data, &window, sizeof(window));
		activity.appContext_->pushEvent(ae);
	}
}
void Activity::onNativeWindowResized(ANativeActivity* nativeActivity,
	ANativeWindow* window) noexcept
{
	auto& activity = retrieveActivity(*nativeActivity);
	std::lock_guard<std::mutex> lock(activity.mutex_);
	if(activity.appContext_) {
		ActivityEvent ae;
		ae.type = ActivityEventType::windowResize;

		auto width = ANativeWindow_getWidth(window);
		auto height = ANativeWindow_getHeight(window);
		std::memcpy(&ae.data[0], &width, 4);
		std::memcpy(&ae.data[4], &height, 4);

		activity.appContext_->pushEvent(ae);
	}
}
void Activity::onNativeWindowRedrawNeeded(ANativeActivity* nativeActivity,
	ANativeWindow*) noexcept
{
	auto& activity = retrieveActivity(*nativeActivity);
	std::lock_guard<std::mutex> lock(activity.mutex_);
	if(activity.appContext_)
		activity.appContext_->pushEventWait({ActivityEventType::windowRedraw});
}
void Activity::onNativeWindowDestroyed(ANativeActivity* nativeActivity,
	ANativeWindow*) noexcept
{
	auto& activity = retrieveActivity(*nativeActivity);
	std::lock_guard<std::mutex> lock(activity.mutex_);
	activity.window_ = nullptr;
	if(activity.appContext_)
		activity.appContext_->pushEventWait({ActivityEventType::windowDestroyed});
}
void Activity::onInputQueueCreated(ANativeActivity* nativeActivity,
	AInputQueue* queue) noexcept
{
	auto& activity = retrieveActivity(*nativeActivity);
	std::lock_guard<std::mutex> lock(activity.mutex_);
	activity.queue_ = queue;
	if(activity.appContext_) {
		ActivityEvent ae;
		ae.type = ActivityEventType::queueCreated;
		std::memcpy(ae.data, &queue, sizeof(queue));
		activity.appContext_->pushEvent(ae);
	}
}
void Activity::onInputQueueDestroyed(ANativeActivity* nativeActivity, AInputQueue*) noexcept
{
	auto& activity = retrieveActivity(*nativeActivity);
	std::lock_guard<std::mutex> lock(activity.mutex_);
	activity.queue_ = nullptr;
	if(activity.appContext_)
		activity.appContext_->pushEventWait({ActivityEventType::queueDestroyed});
}

// Activity
Activity::~Activity()
{
	if(valid()) {
		ny::log("ny::android::~Activity: destroy not called");
		activity_->instance = nullptr;
	}

	mainThread_.join();
}

void Activity::init(ANativeActivity& nativeActivity)
{
	// init the logs to use android-log
	debugLogger() = std::make_unique<AndroidLogger>(ANDROID_LOG_DEBUG, "ny::debug");
	logLogger() = std::make_unique<AndroidLogger>(ANDROID_LOG_INFO, "ny::log");
	warningLogger() = std::make_unique<AndroidLogger>(ANDROID_LOG_WARN, "ny::warning");
	errorLogger() = std::make_unique<AndroidLogger>(ANDROID_LOG_ERROR, "ny::error");

	log("ny::android: initialized");

	// set all needed callbacks
	activity_ = &nativeActivity;
	nativeActivity.instance = this;

	auto& cb = *nativeActivity.callbacks;
	cb.onStart = &Activity::onStart;
	cb.onResume = &Activity::onResume;
	cb.onPause = &Activity::onPause;
	cb.onStop = &Activity::onStop;
	cb.onDestroy = &Activity::onDestroy;
	cb.onWindowFocusChanged = &Activity::onWindowFocusChanged;
	cb.onNativeWindowCreated = &Activity::onNativeWindowCreated;
	cb.onNativeWindowDestroyed = &Activity::onNativeWindowDestroyed;
	cb.onNativeWindowRedrawNeeded = &Activity::onNativeWindowRedrawNeeded;
	cb.onNativeWindowResized = &Activity::onNativeWindowResized;
	cb.onInputQueueCreated = &Activity::onInputQueueCreated;
	cb.onInputQueueDestroyed = &Activity::onInputQueueDestroyed;

	// start the main thread
	auto mainWrapper = [&]{ this->mainThreadFunction(); };
	mainThread_ = std::thread(mainWrapper);
}

void Activity::destroy()
{
	if(!valid()) {
		warning("ny::android::Activity::destroy: not valid");
		return;
	}

	// cleanup
	activity_ = {};
	window_ = {};
	queue_ = {};
}

bool Activity::valid() const
{
	std::lock_guard<std::mutex> lock(mutex());
	return (activity_);
}

void Activity::mainThreadFunction()
{
	// call the applications main method
	// since this is the main thread function we output exceptions
	// we always have to call ANativeActivity_stop
	try {
		::main(0, nullptr);
	} catch(const std::exception& err) {
		error("ny::android: main function exception: ", err.what());
	} catch(...) {
		error("ny::android: main function threw unknown exception object");
	}

	log("ny::android: Exiting main thread, finishing activity");

	// stop the native activity
	// this makes the application quit
	// needed since we run main not in the main thread
	// the main thread belongs to the activity
	ANativeActivity_finish(activity_);
}

// utility
Activity& retrieveActivity(const ANativeActivity& nativeActivity) noexcept
{
	return *static_cast<Activity*>(nativeActivity.instance);
}

} // namespace ny::android

// The native activity entry point
// This is where the application is started
// we only construct the singleton android activity object that will
// initialize the main thread and callbacks
extern "C"
void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	nytl::unused(savedState, savedStateSize);
	ny::android::Activity::instanceUnchecked().init(*activity);
}
