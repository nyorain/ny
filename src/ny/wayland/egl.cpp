// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/egl.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/util.hpp>
#include <ny/common/egl.hpp>
#include <ny/surface.hpp>
#include <dlg/dlg.hpp>

#include <wayland-egl.h>
#include <EGL/egl.h>

namespace ny {

// WaylandEglWindowContext
WaylandEglWindowContext::WaylandEglWindowContext(WaylandAppContext& ac, const EglSetup& setup,
	const WaylandWindowSettings& ws) : WaylandWindowContext(ac, ws)
{
	auto size = ws.size;
	if(size == defaultSize) size = fallbackSize;

	wlEglWindow_ = wl_egl_window_create(&wlSurface(), size[0], size[1]);
	if(!wlEglWindow_) {
		throw std::runtime_error("ny::WaylandEglWindowContext: wl_egl_window_create failed");
	}

	auto eglnwindow = static_cast<void*>(wlEglWindow_);
	surface_ = std::make_unique<EglSurface>(setup, eglnwindow, ws.gl.config);
	if(ws.gl.storeSurface) *ws.gl.storeSurface = surface_.get();
}

WaylandEglWindowContext::~WaylandEglWindowContext()
{
	surface_.reset();
	if(wlEglWindow_) {
		wl_egl_window_destroy(wlEglWindow_);
	}
}

void WaylandEglWindowContext::size(nytl::Vec2ui size)
{
	WaylandWindowContext::size(size);
	wl_egl_window_resize(wlEglWindow_, size[0], size[1], 0, 0);
}

Surface WaylandEglWindowContext::surface()
{
	return {*surface_};
}

} // namespace ny
