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
//   since this might end badly considering android general
//   poor c++ exception support
//   Logging the message is the best we can do

// NOTE: we wait until the native window is created until we start the
//   main thread. This will minimize the change to create a WindowContext
//   without native window which may trigger application bugs.

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
	return instanceUnchecked().get();
	return nullptr;
}

std::unique_ptr<Activity>& Activity::instanceUnchecked()
{
	static std::unique_ptr<Activity> singleton;
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
	auto& activity = retrieveActivity(*nativeActivity);
	log("ny::android: onDestroy");

	// send the destroy event to the app context and wait for processing
	// this will end any event processing by app context
	{
		std::lock_guard<std::mutex> lock(activity.mutex_);
		if(activity.appContext_)
			activity.appContext_->pushEventWait({ActivityEventType::destroy});
	}

	// delete the Activity global
	// this will wait for the main thread to finish
	instanceUnchecked().reset();
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
	log("ny::android: native window created");

	{
		std::lock_guard<std::mutex> lock(activity.mutex_);
		activity.window_ = window;
		if(activity.appContext_) {
			ActivityEvent ae;
			ae.type = ActivityEventType::windowCreated;
			std::memcpy(ae.data, &window, sizeof(window));
			activity.appContext_->pushEvent(ae);
		}
	}

	// this will check if the thread is already initialized
	activity.initMainThread();
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
Activity::Activity(ANativeActivity& nativeActivity)
{
	// init the logs to use android-log
	debugLogger() = std::make_unique<AndroidLogger>(ANDROID_LOG_DEBUG, "ny");
	logLogger() = std::make_unique<AndroidLogger>(ANDROID_LOG_INFO, "ny");
	warningLogger() = std::make_unique<AndroidLogger>(ANDROID_LOG_WARN, "ny");
	errorLogger() = std::make_unique<AndroidLogger>(ANDROID_LOG_ERROR, "ny");

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

	mainRunning_.store(false);
	log("ny::android::Activity: initialized");
}

Activity::~Activity()
{
	log("ny::android::~Activity: reached");

	// assure main thread terminates
	if(mainRunning_.load()) {
		std::unique_lock<std::mutex> lock(mutex_);
		mainThreadCV_.wait(lock, [&]{ return !mainRunning_.load(); });
	}

	activity_->instance = nullptr;
	log("ny::android::~Activity: reached");
}

void Activity::initMainThread()
{
	if(mainRunning_.load())
		return;

	auto mainWrapper = [&]{ this->mainThreadFunction(); };
	std::thread mainThread(mainWrapper);
	mainThread.detach();

	// wait until thread is started
	if(!mainRunning_.load()) {
		std::unique_lock<std::mutex> lock(mutex_);
		mainThreadCV_.wait(lock, [&]{ return mainRunning_.load(); });
	}

	log("ny::android::Activity: main thread started");
}

void Activity::mainThreadFunction()
{
	log("ny::android::Activity: Starting main thread");
	mainRunning_.store(true);
	mainThreadCV_.notify_one();

	// call the applications main method
	// since this is the main thread function we output exceptions
	try {
		::main(0, nullptr);
	} catch(const std::exception& err) {
		error("ny::android: main function exception: ", err.what());
	} catch(...) {
		error("ny::android: main function threw unknown exception object");
	}

	mainRunning_.store(false);
	mainThreadCV_.notify_one();
	log("ny::android::Activity: Exiting main thread");
}

// utility
Activity& retrieveActivity(const ANativeActivity& nativeActivity) noexcept
{
	if(!nativeActivity.instance) {
		error("android: instance == nullptr. This should not happen.");
		std::terminate();
	}

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
	using ny::android::Activity;
	// if(Activity::instance()) {
	// 	ny::warning("there is already an instance...");
	// 	// ANativeActivity_finish(Activity::instance()->activity_);
	// 	Activity::onDestroy(Activity::instance()->activity_);
	//
	// 	if(Activity::instance() && Activity::instance()->mainRunning_) {
	// 		std::unique_lock<std::mutex> lock(Activity::instance()->mutex_);
	// 		Activity::instance()->mainThreadCV_.wait(lock, [&]{ return Activity::instance() == nullptr || !Activity::instance()->mainRunning_.load(); });
	// 		ny::warning("finished waiting...");
	// 	}
	// }

	Activity::instanceUnchecked().release();
	nytl::unused(savedState, savedStateSize);
	Activity::instanceUnchecked().reset(new Activity(*activity));
	// Activity::instance()->initMainThread();
}
