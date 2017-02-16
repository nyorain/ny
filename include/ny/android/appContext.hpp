// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <ny/appContext.hpp>

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

	bool dispatchEvents() override;
	bool dispatchLoop(LoopControl&) override;

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

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace ny
