// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/activity.hpp>

// External main declaration
// implemented by application
extern int main(int, char**);

namespace ny::android {

// we make this a unique pointer global to assure it is deleted
// when the program terminates
// We will also reset the unique pointer in the native activity destroy
// callback but that might not be called in all cases
std::unique_ptr<ActivityState> activityState {};

// Activity - static
Activity* Activity::instanceFunc(Activity* acitivty, bool set)
{
	static Activity* singleton;
	if(set) singleton = activity;
	return singleton;
}

void Activity::onStart(ANativeActivity*)
{

}
void Activity::onResume(ANativeActivity*)
{

}
void Activity::onPause(ANativeActivity*)
{

}
void Activity::onStop(ANativeActivity*)
{

}
void Activity::onDestroy(ANativeActivity*)
{

}
void Activity::onWindowFocusChanged(ANativeActivity*, int hasFocus)
{

}
void Activity::onNativeWindowCreated(ANativeActivity*, ANativeWindow*)
{

}
void Activity::onNativeWindowResized(ANativeActivity*, ANativeWindow*)
{

}
void Activity::onNativeWindowRedrawNeeded(ANativeActivity*, ANativeWindow*)
{

}
void Activity::onNativeWindowDestroyed(ANativeActivity*, ANativeWindow*)
{

}
void Activity::onInputQueueCreated(ANativeActivity*, AInputQueue*)
{

}
void Activity::onInputQueueDestroyed(ANativeActivity*, AInputQueue*)
{

}

// Activity
Activity(ANativeActivity& nativeActivity) : nativeActivity_(nativeActivity)
{
	// set all needed callbacks
	auto cb = *nativeActivity.callbacks;
	cb.onStart = onStart;
	cb.onResume = onResume;
	cb.onPause = onPause;
	cb.onStop = onStop;
	cb.onDestroy = onDestroy;
	cb.onWindowFocusChanged = onWindowFocusChanged;
	cb.onNativeWindowCreated = onNativeWindowCreated;
	cb.onNativeWindowDestroyed = onNativeWindowDestroyed;
	cb.onNativeWindowRedrawNeeded = onNativeWindowRedrawNeeded;
	cb.onNativeWindowResized = onNativeWindowResized;
	cb.onInputQueueCreated = onInputQueueCreated;
	cb.onInputQueueDestroyed = onInputQueueDestroyed;

	// start the main thread
	auto func = [&]{ this->mainThreadFunction(); };
	mainThread_ = std::thread(func);
}

~Activity()
{
	activity_.instance = nullptr;
}

void Activity::mainThreadFunction()
{

}

// utility
Activity& retrieveActivity(const ANativeActivity& nativeActivity)
{
	return *static_cast<Activity&>(nativeActivity->instance);
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
	ny::android::activityState = std::make_unique<ny::android::AcitivityState>(*activity);
}
