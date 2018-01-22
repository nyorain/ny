// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <ny/windowContext.hpp>
#include <ny/windowSettings.hpp>
#include <android/native_window.h>

namespace ny {

struct AndroidWindowSettings : public WindowSettings {};

/// Android WindowContext implementation.
/// Cannot implement many of the desktop-orientated functions correctly, i.e. has
/// few capabilities.
/// Note that it might be an empty wrapper when there is currently no native
/// window for the activity.
/// The reference native window (and therefore its native handle) might also
/// change during its lifetime.
class AndroidWindowContext : public WindowContext {
public:
	AndroidWindowContext(AndroidAppContext& ac, const AndroidWindowSettings& settings);
	~AndroidWindowContext();

	WindowCapabilities capabilities() const override;
	Surface surface() override;
	NativeHandle nativeHandle() const override { return nativeWindow(); }

	void show() override;
	void hide() override;

	void minSize(nytl::Vec2ui minSize) override;
	void maxSize(nytl::Vec2ui maxSize) override;

	void size(nytl::Vec2ui size) override;
	void position(nytl::Vec2i position) override;

	void cursor(const Cursor& c) override;
	void refresh() override;

	void maximize() override;
	void minimize() override;
	void fullscreen() override;
	void normalState() override;

	void beginMove(const EventData*) override;
	void beginResize(const EventData* event, WindowEdges edges) override;

	void title(const char* name) override;
	void icon(const Image& newicon) override;
	void customDecorated(bool set) override;
	bool customDecorated() const override { return false; }

	// - android specific -
	ANativeWindow* nativeWindow() const { return nativeWindow_; }

protected:
	/// Resets the associated native window.
	/// Called when the native window is destroyed or a new one is created.
	/// Will set the nativeWindow_ member to nullptr.
	/// Derived implementations must make sure that the previous
	/// native window or resources created for it are no longer accessed or
	/// create new resources.
	/// Might be nullptr if the native window is destroyed.
	virtual void nativeWindow(ANativeWindow*);
	friend class AndroidAppContext;

protected:
	AndroidAppContext& appContext_;
	ANativeWindow* nativeWindow_;
};

} // namespace ny
