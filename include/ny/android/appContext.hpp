// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <ny/android/activity.hpp>
#include <ny/appContext.hpp>

#include <android/native_activity.h>
#include <android/looper.h>

namespace ny {

/// Android AppContext implementation.
class AndroidAppContext : public AppContext {
public:
	AndroidAppContext(android::Activity& activity);
	~AndroidAppContext();

	WindowContextPtr createWindowContext(const WindowSettings& settings) override;
	MouseContext* mouseContext() override { return nullptr; }
	KeyboardContext* keyboardContext() override { return nullptr; }

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
	ANativeActivity& nativeActivity() const { return *nativeActivity_; }
	ANativeWindow& nativeWindow() const { return *activity().nativeWindow(); }

protected:
	friend class android::Activity;
	void windowFocusChanged(bool gained);
	void windowResized();
	void windowRedrawNeeded();
	void windowDestroyed();
	void activityDestroyed();

protected:
	android::Activity& activity_;
	AndroidWindowContext* windowContext_ {};

	ANativeActivity* nativeActivity_ {};
	ALooper* looper_ {};
	AInputQueue* inputQueue_ {};

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace ny
