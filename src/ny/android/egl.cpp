// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/egl.hpp>
#include <ny/android/appContext.hpp>
#include <ny/common/egl.hpp>
#include <ny/surface.hpp>
#include <ny/log.hpp>

#include <EGL/egl.h>

namespace ny {

AndroidEglWindowContext::AndroidEglWindowContext(AndroidAppContext& ac, EglSetup& setup,
	const AndroidWindowSettings& ws) : AndroidWindowContext(ac, ws)
{
	glConfig_ = ws.gl.config;
	if(!glConfig_) glConfig_ = setup.defaultConfig().id;

	// could not find any doc that this is needed but everyone seems to do it
	auto config = setup.eglConfig(glConfig_);
	if(!::eglGetConfigAttrib(setup.eglDisplay(), config, EGL_NATIVE_VISUAL_ID, &format_)) {
		std::string msg = "ny::AndroidEglWindowContext: could not retrieve config format";
		throw EglErrorCategory::exception(msg);
	}

	if(!nativeWindow()) {
		warning("ny::AndroidEglWindowContext: no native window");
		if(ws.gl.storeSurface) *ws.gl.storeSurface = nullptr;
		return;
	}

	ANativeWindow_setBuffersGeometry(nativeWindow(), 0, 0, format_);

	// create the surface
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

		// the surfaceDestroyed handler must make the surface not
		// current, we just check here for debugging purposes
		if(surface_->isCurrent()) {
			constexpr auto funcName = "ny::AndroidEglWindowContext::nativeWindow";
			error(funcName, "surfaceDestroyed handler did not make the surface not current");
		}

		surface_.reset();
	}

	if(window) {
		auto androidnwindow = static_cast<void*>(nativeWindow());
		auto& setup = *appContext_.eglSetup();

		ANativeWindow_setBuffersGeometry(nativeWindow(), 0, 0, format_);
		surface_ = std::make_unique<EglSurface>(setup, androidnwindow, glConfig_);

		SurfaceCreatedEvent sce;
		sce.surface = {*surface_};
		listener().surfaceCreated(sce);
	}
}

} // namespace ny
