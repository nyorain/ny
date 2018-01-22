// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>

#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

/// Fwd decl, imlpemented in activity.cpp.
/// Needed for Activity friend declaration.
extern "C" void ANativeActivity_onCreate(ANativeActivity*, void*, size_t);

namespace ny::android {

/// Holds all state associated to the android NativeActivity.
/// Can be used to retrieve android specific objects.
/// In an android application, there exists a singleton object of this class.
/// Cannot be manually created.
/// Cannot be changed from the outside once created valid.
/// Can be read from multiple threads at the same time.
/// Will exist before an AppContext is created and store the native objects
/// retrieved by callbacks so the AppContext can use it no matter
/// when it will be created.
class Activity final {
public:
	/// Returns the Activity singleton or nullptr if there is none.
	/// If the application currently runs ons android then this will return
	/// the same (valid) pointer during the scope of main.
	static Activity* instance();

public:
	~Activity();

	/// Can be used to get the retrieved native objects.
	/// The mutex returned by mutex() must be locked while retrieving and
	/// using them since otherwise they may become invalid.
	ANativeActivity* nativeActivity() const { return activity_; }
	ANativeWindow* nativeWindow() const { return window_; }
	AInputQueue* inputQueue() const { return queue_; }

	/// Returns the internal synchronization mutex.
	/// This is locked everytime the internal native variables are accesed
	/// and can be locked from the outside to assure that those variables
	/// don't change (e.g. to keep the window alive until unlocked).
	/// Note that no blocking operations should be done while locking the mutex.
	std::mutex& mutex() const { return mutex_; }

private:
	Activity(ANativeActivity&, void* state, std::size_t stateSize);

	void mainThreadFunction();
	void initMainThread();

private:
	/// The Activity singleton.
	/// Does not check if it is valid (needed for initialization).
	static std::atomic<Activity*>& instanceRef();

	// native activity callback functions
	// will receive the Activity singleton and change it or
	// dispatch events to the event queue of the associated appContext
	static void onStart(ANativeActivity*) noexcept;
	static void onResume(ANativeActivity*) noexcept;
	static void onPause(ANativeActivity*) noexcept;
	static void onStop(ANativeActivity*) noexcept;
	static void onDestroy(ANativeActivity*) noexcept;
	static void onWindowFocusChanged(ANativeActivity*, int hasFocus) noexcept;
	static void onNativeWindowCreated(ANativeActivity*, ANativeWindow*) noexcept;
	static void onNativeWindowResized(ANativeActivity*, ANativeWindow*) noexcept;
	static void onNativeWindowRedrawNeeded(ANativeActivity*, ANativeWindow*) noexcept;
	static void onNativeWindowDestroyed(ANativeActivity*, ANativeWindow*) noexcept;
	static void onInputQueueCreated(ANativeActivity*, AInputQueue*) noexcept;
	static void onInputQueueDestroyed(ANativeActivity*, AInputQueue*) noexcept;
	static void* onSaveInstance(ANativeActivity*, std::size_t*) noexcept;

private:
	ANativeActivity* activity_ {};
	ANativeWindow* window_ {};
	AInputQueue* queue_ {};

	std::vector<uint8_t> savedState_ {};
	AndroidAppContext* appContext_ {};
	std::condition_variable mainThreadCV_ {};
	std::atomic<bool> mainRunning_ {};
	mutable std::mutex mutex_ {};

	friend class ny::AndroidAppContext;
	friend void ::ANativeActivity_onCreate(ANativeActivity*, void*, size_t);
};

/// The type of an activity event sent from a callback
/// in the activity thread to the main event thread.
enum class ActivityEventType {
	windowDestroyed,
	windowCreated,
	windowFocusChanged,
	windowRedraw,
	windowResize,
	windowFocus,
	queueCreated,
	queueDestroyed,
	saveState,
	start,
	pause,
	resume,
	stop,
	destroy
};

/// Event coming directly from the activity callbacks.
/// Used for the AppContexts
struct ActivityEvent {
	// the type of the event
	ActivityEventType type {};

	// possibility to transmit event-specific data
	// interpretation dependent on the event type
	// has size for 2 pointers (more is not needed)
	uint8_t data[sizeof(void*) * 2] {};

	// if this is not nullptr, the main thread will
	// call notify_one on the eventQueueCV when
	// finishing to handle the event and will
	// set the atomic bool to true
	std::atomic<bool>* signalCV {};
};

/// Retrieves the associated Activity object from a ANativeActivity.
/// Undefined behvaiour if no Activity was associated to it or it
/// was already deleted (should never happen).
Activity& retrieveActivity(const ANativeActivity& nativeActivity) noexcept;

} // namespace ny::android
