// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <ny/android/windowContext.hpp>

namespace ny {

class AndroidEglWindowContext : public AndroidWindowContext {
public:
	AndroidEglWindowContext(AndroidAppContext&, EglSetup&, const AndroidWindowSettings&);
	~AndroidEglWindowContext() = default;

	Surface surface() override;
	EglSurface& eglSurface() const { return *surface_; }

protected:
	std::unique_ptr<EglSurface> surface_;
};

} // namespace ny

#ifndef NY_WithEgl
	#error ny was built without egl. Do not include this header.
#endif
