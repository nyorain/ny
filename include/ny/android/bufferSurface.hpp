// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <ny/android/windowContext.hpp>
#include <ny/bufferSurface.hpp>

#include <nytl/vec.hpp>

#include <android/native_window.h>

namespace ny {

/// Android BufferSurface implementatoin.
class AndroidBufferSurface : public BufferSurface {
public:
	AndroidBufferSurface(ANativeWindow&);
	~AndroidBufferSurface();

	BufferGuard buffer() override;

protected:
	void apply(const BufferGuard&) noexcept override;

protected:
	ANativeWindow& nativeWindow_;
	ANativeWindow_Buffer buffer_ {};
};

/// Android WindowContext implementation that holds a BufferSurface.
class AndroidBufferWindowContext : public AndroidWindowContext {
public:
	AndroidBufferWindowContext(AndroidAppContext&, const AndroidWindowSettings& = {});
	~AndroidBufferWindowContext() = default;

	Surface surface() noexcept override;
	using AndroidWindowContext::nativeWindow;

protected:
	void nativeWindow(ANativeWindow*) override;

protected:
	std::unique_ptr<AndroidBufferSurface> bufferSurface_;
};

} // namespace ny
