// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/activity.hpp>
#include <ny/android/appContext.hpp>
#include <ny/log.hpp>
#include <nytl/tmpUtil.hpp>

#include <android/log.h>

// NOTE: we will never throw from inside the callbacks
// since this might end badly considering android general
// poor c++ exception support
// Logging the message is the best we can do

// NOTE: we wait until the native window is created until
// we create and start the main thread to make sure we will
// never have to create a WindowContext in AppContext without
// having the native window retrieved yet

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

void Activity::onStart(ANativeActivity*)
{
	// not interesting atm
}
void Activity::onResume(ANativeActivity*)
{
	// not interesting atm
}
void Activity::onPause(ANativeActivity*)
{
	// not interesting atm
}
void Activity::onStop(ANativeActivity*)
{
	// not interesting atm
}
void Activity::onDestroy(ANativeActivity* nativeActivity)
{
	// make sure the NativeActivity is accessed no longer
	auto& activity = retrieveActivity(*nativeActivity);
	activity.destroy();
}
void Activity::onWindowFocusChanged(ANativeActivity* nativeActivity, int hasFocus)
{
	auto& activity = retrieveActivity(*nativeActivity);
	auto appContext = activity.appContext_.load();

	if(appContext)
		appContext->windowFocusChanged(hasFocus);
}
void Activity::onNativeWindowCreated(ANativeActivity* nativeActivity,
	ANativeWindow* window)
{
	log("native window created");
	auto& activity = retrieveActivity(*nativeActivity);
	activity.window_ = window;

	// now we start the main thread
	auto mainWrapper = [&]{ activity.mainThreadFunction(); };
	activity.mainThread_ = std::thread(mainWrapper);
}
void Activity::onNativeWindowResized(ANativeActivity* nativeActivity,
	ANativeWindow*)
{
	auto& activity = retrieveActivity(*nativeActivity);
	auto appContext = activity.appContext_.load();
	if(appContext)
		appContext->windowResized();
}
void Activity::onNativeWindowRedrawNeeded(ANativeActivity* nativeActivity,
	ANativeWindow*)
{
	auto& activity = retrieveActivity(*nativeActivity);
	auto appContext = activity.appContext_.load();
	if(appContext)
		appContext->windowRedrawNeeded();
}
void Activity::onNativeWindowDestroyed(ANativeActivity* nativeActivity,
	ANativeWindow*)
{
	auto& activity = retrieveActivity(*nativeActivity);
	auto appContext = activity.appContext_.load();
	if(appContext)
		appContext->windowDestroyed();
}
void Activity::onInputQueueCreated(ANativeActivity* nativeActivity,
	AInputQueue* queue)
{
	auto& activity = retrieveActivity(*nativeActivity);
	activity.queue_.store(queue);
}
void Activity::onInputQueueDestroyed(ANativeActivity* nativeActivity, AInputQueue*)
{
	auto& activity = retrieveActivity(*nativeActivity);
	activity.queue_.store(nullptr);
}

// Activity
Activity::~Activity()
{
	if(valid()) {
		ny::log("ny::android::~Activity: destroy not called");
		activity_.load()->instance = nullptr;
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

	ny::log("ny::android: initialized");

	// set all needed callbacks
	activity_.store(&nativeActivity);
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
}

void Activity::destroy()
{
	if(!valid()) {
		ny::warning("ny::android::Acitivity::destroy: not valid");
		return;
	}

	// signal the appContext
	// it should access the activity no further and
	// signal that it cannot be longer used
	auto appContext = appContext_.load();
	if(appContext)
		appContext->activityDestroyed();

	// cleanup
	activity_.store(nullptr);
	window_.store(nullptr);
	queue_.store(nullptr);
	appContext_.store(nullptr);
}

void Activity::mainThreadFunction()
{
	// call the applications main method
	// since this is the main thread function we output exceptions
	// we always have to call ANativeActivity_stop
	try {
		::main(0, nullptr);
	} catch(const std::exception& error) {
		ny::error("ny::android: main function exception: ", error.what());
	} catch(...) {
		ny::error("ny::android: main function threw unknown exception object");
	}

	ny::log("ny::android: Exiting main thread, finishing activity");

	// stop the native activity
	// this makes the application quit
	// needed since we run main not in the main thread
	// the main thread belongs to the activity
	ANativeActivity_finish(activity_.load());
}

// utility
Activity& retrieveActivity(const ANativeActivity& nativeActivity)
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
