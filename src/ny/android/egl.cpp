// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/egl.hpp>
#include <ny/common/egl.hpp>
#include <ny/surface.hpp>

namespace ny {

AndroidEglWindowContext::AndroidEglWindowContext(AndroidAppContext& ac, EglSetup& setup,
	const AndroidWindowSettings& ws) : AndroidWindowContext(ac, ws)
{
	auto androidnwindow = static_cast<void*>(nativeWindow());
	surface_ = std::make_unique<EglSurface>(setup.eglDisplay(), androidnwindow, ws.gl.config);

	if(ws.gl.storeSurface) *ws.gl.storeSurface = surface_.get();
}

Surface AndroidEglWindowContext::surface() noexcept
{
	return {*surface_};
}

} // namespace ny
