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
	AndroidBufferSurface(AndroidWindowContext&);
	~AndroidBufferSurface() = default;

	BufferGuard buffer() override;
	void apply(const BufferGuard&) noexcept override;

protected:
	AndroidWindowContext& windowContext_;
	ANativeWindow_Buffer buffer_;
};

/// Android WindowContext implementation that holds a BufferSurface.
class AndroidBufferWindowContext : public AndroidWindowContext {
public:
	AndroidBufferWindowContext(AndroidAppContext&, const AndroidWindowSettings& = {});
	~AndroidBufferWindowContext() = default;

	Surface surface() noexcept override;

protected:
	AndroidBufferSurface bufferSurface_;
};

} // namespace ny
