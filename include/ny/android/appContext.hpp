// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <ny/android/activity.hpp>
#include <ny/android/input.hpp>
#include <ny/appContext.hpp>

#include <android/native_activity.h>
#include <android/looper.h>

namespace ny {

/// Android AppContext implementation.
/// Manages the ALooper event dispatching, communicates with the activity
/// thread and handles its activity events.
class AndroidAppContext : public AppContext {
public:
	AndroidAppContext(android::Activity& activity);
	~AndroidAppContext();

	WindowContextPtr createWindowContext(const WindowSettings& settings) override;
	MouseContext* mouseContext() override { return &mouseContext_; }
	KeyboardContext* keyboardContext() override { return &keyboardContext_; }

	bool dispatchEvents() override;
	bool dispatchLoop(LoopControl&) override;

	bool clipboard(std::unique_ptr<DataSource>&&) override { return false; }
	DataOffer* clipboard() override { return nullptr; }
	bool startDragDrop(std::unique_ptr<DataSource>&&) override { return false; }

	std::vector<const char*> vulkanExtensions() const override;
	GlSetup* glSetup() const override;

	// - android specific -
	EglSetup* eglSetup() const;
	android::Activity& activity() const { return activity_; }
	ANativeActivity* nativeActivity() const { return nativeActivity_; }
	ANativeWindow* nativeWindow() const { return nativeWindow_; }
	AndroidWindowContext* windowContext() const { return windowContext_; }

	bool showKeyboard() const;
	bool hideKeyboard() const;

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

	AndroidMouseContext mouseContext_;
	AndroidKeyboardContext keyboardContext_;

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace ny
