// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/egl.hpp>
#include <ny/android/appContext.hpp>
#include <ny/common/egl.hpp>
#include <ny/surface.hpp>
#include <ny/log.hpp>

namespace ny {

AndroidEglWindowContext::AndroidEglWindowContext(AndroidAppContext& ac, EglSetup& setup,
	const AndroidWindowSettings& ws) : AndroidWindowContext(ac, ws)
{
	glConfig_ = ws.gl.config;
	if(!nativeWindow()) {
		warning("ny::AndroidEglWindowContext: no native window");
		if(ws.gl.storeSurface) *ws.gl.storeSurface = nullptr;
		return;
	}

	auto androidnwindow = static_cast<void*>(nativeWindow());
	surface_ = std::make_unique<EglSurface>(setup, androidnwindow, glConfig_);

	if(ws.gl.storeSurface) *ws.gl.storeSurface = surface_.get();
}

Surface AndroidEglWindowContext::surface()
{
	return {*surface_};
}

void AndroidEglWindowContext::nativeWindow(ANativeWindow* window)
{
	AndroidWindowContext::nativeWindow(window);
	if(surface_) {
		SurfaceDestroyedEvent sde;
		listener().surfaceDestroyed(sde);
		surface_.reset();
	}

	if(window) {
		auto androidnwindow = static_cast<void*>(nativeWindow());
		auto& setup = *appContext_.eglSetup();
		surface_ = std::make_unique<EglSurface>(setup, androidnwindow, glConfig_);

		SurfaceCreatedEvent sce;
		sce.surface = {*surface_};
		listener().surfaceCreated(sce);
	}
}

} // namespace ny
