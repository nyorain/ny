// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <ny/appContext.hpp>
#include <functional>

namespace ny {

/// Android AppContext implementation.
/// Manages the ALooper event dispatching, communicates with the activity
/// thread and handles its activity events.
class AndroidAppContext : public AppContext {
public:
	AndroidAppContext(android::Activity& activity);
	~AndroidAppContext();

	WindowContextPtr createWindowContext(const WindowSettings& settings) override;
	MouseContext* mouseContext() override;
	KeyboardContext* keyboardContext() override;

	bool pollEvents() override;
	bool waitEvents() override;
	void wakeupWait() override;

	bool clipboard(std::unique_ptr<DataSource>&&) override { return false; }
	DataOffer* clipboard() override { return nullptr; }
	bool startDragDrop(std::unique_ptr<DataSource>&&) override { return false; }

	std::vector<const char*> vulkanExtensions() const override;
	GlSetup* glSetup() const override;

	// - android specific -
	EglSetup* eglSetup() const;
	ANativeActivity* nativeActivity() const { return nativeActivity_; }
	ANativeWindow* nativeWindow() const { return nativeWindow_; }
	AndroidWindowContext* windowContext() const { return windowContext_; }
	JNIEnv* jniEnv() const { return jniEnv_; }
	android::Activity& activity() const { return activity_; }

	/// Sets the function that implements the state save.
	/// The saved state can than be retrieved on startup from the Android
	/// backend. The saver must return data allocated using alloc (new should work)
	/// and set the passed size parameter to the size of the returned data.
	/// It may return nullptr and set the size to 0.
	void stateSaver(const std::function<void*(std::size_t&)>& saver);

	/// Returns the saved state that was previously saved by the application
	/// using the state saver functionality.
	/// Note that the returned vector might be empty.
	/// The first time this is called, the saved state is moved into
	/// the returned vector, so any call after the first one will
	/// definietly receive an empty vector.
	std::vector<uint8_t> savedState();

	// - direct native activity wrappers for convinience -
	// shows/hides the soft input
	// see the native_activity documentation
	// Will return false if it cannot be executed since
	// the native activity was already destroyed
	bool showKeyboard() const;
	bool hideKeyboard() const;

	// returns the apps data paths
	// see the native_activity documentation
	// returns just an empty string if the native
	// activity was already destroyed
	const char* internalPath() const;
	const char* externalPath() const;

protected:
	void handleActivityEvents();
	void inputReceived();
	void initQueue();

	void windowFocusChanged(bool gained);
	void windowCreated(ANativeWindow&);
	void inputQueueCreated(AInputQueue&);
	void windowResized(nytl::Vec2i32 size);
	void inputQueueDestroyed();
	void windowDestroyed();
	void windowRedrawNeeded();
	void activityDestroyed();

	friend class android::Activity;
	void pushEvent(const android::ActivityEvent& ev) noexcept;
	void pushEventWait(android::ActivityEvent ev) noexcept;

	friend class AndroidWindowContext;
	void windowContextDestroyed();

protected:
	android::Activity& activity_;
	AndroidWindowContext* windowContext_ {};

	ANativeActivity* nativeActivity_ {};
	ANativeWindow* nativeWindow_ {};
	ALooper* looper_ {};
	AInputQueue* inputQueue_ {};
	JNIEnv* jniEnv_ {};

	std::unique_ptr<AndroidMouseContext> mouseContext_;
	std::unique_ptr<AndroidKeyboardContext> keyboardContext_;
	std::function<void*(std::size_t&)> stateSaver_;

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace ny
