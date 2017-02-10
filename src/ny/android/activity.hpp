// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <android/native_activity.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace ny::android {

/// Holds all state associated to the android NativeActivity.
/// Can be used to retrieve android specific objects.
/// In an android application, there exists a singleton object of this class.
/// Cannot be manually created.
/// Cannot be changed from the outside once created valid.
/// Can be read from multiple threads at the same time.
/// Uses internally atomic variables.
class Activity final {
public:
	/// Returns the Activity singleton or nullptr if there is none.
	/// If the application currently runs ons android then this will return
	/// the same (valid) pointer during the scope of main.
	static Activity* instance();

public:
	ANativeActivity& nativeAcitivy() const { return *activity_; }
	ANativeWindow* nativeWidnow() const { return window_; }
	AInputQueue* inputQueue() const { return queue_; }
	const std::thread& mainThread() const { return mainThread_; }

private:
	Activity() = default; // constructs invalid
	~Activity();

	void init(ANativeActivity&);
	void destroy();

	bool valid() const { return acitivty.load(); };
	void mainThreadFunction();

private:
	/// The Activity singleton.
	/// Does not check if it is valid (needed for initialization).
	static Activity& instanceUnchecked();

	static void onStart(ANativeActivity*);
	static void onResume(ANativeActivity*);
	static void onPause(ANativeActivity*);
	static void onStop(ANativeActivity*);
	static void onDestroy(ANativeActivity*);
	static void onWindowFocusChanged(ANativeActivity*, int hasFocus);
	static void onNativeWindowCreated(ANativeActivity*, ANativeWindow*);
	static void onNativeWindowResized(ANativeActivity*, ANativeWindow*);
	static void onNativeWindowRedrawNeeded(ANativeActivity*, ANativeWindow*);
	static void onNativeWindowDestroyed(ANativeActivity*, ANativeWindow*);
	static void onInputQueueCreated(ANativeActivity*, AInputQueue*);
	static void onInputQueueDestroyed(ANativeActivity*, AInputQueue*);

private:
	std::atomic<ANativeActivity*> activity_ {};
	std::atomic<ANativeWindow*> window_ {};
	std::atomic<AInputQueue*> queue_ {};
	std::atomic<AndroidAppContext*> appContext_ {};
	std::thread mainThread_ {};

	friend void ANativeActivity_onCreate(ANativeActivity*, void*, size_t);
};

/// Retrieves the associated Activity object from a ANativeActivity.
/// Undefined behvaiour if no Activity was associated to it or it
/// was already deleted (should never happen).
Activity& retrieveActivity(const ANativeActivity& nativeActivity);

} // namespace ny::android
