// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <ny/windowContext.hpp>
#include <ny/windowSettings.hpp>
#include <android/native_window.h>

namespace ny {

struct AndroidWindowSettings : public WindowSettings {};

///Android WindowContext implementation.
///Cannot implement many of the desktop-orientated functions correctly, i.e. has
///few capabilities.
///Wrapper around ANativeWindow functionality.
class AndroidWindowContext : public WindowContext {
public:
	AndroidWindowContext(AndroidAppContext& ac, const AndroidWindowSettings& settings);
	~AndroidWindowContext();

	// - android specific -
	Surface surface() override;
	ANativeWindow& nativeWindow() const { return *nativeWindow_; }

protected:
	ANativeWindow* nativeWindow_;
};

} // namespace ny
