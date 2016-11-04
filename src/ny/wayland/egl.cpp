#include <ny/backend/wayland/egl.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/wayland/util.hpp>
#include <ny/backend/integration/surface.hpp>
#include <ny/base/log.hpp>

#include <wayland-egl.h>
#include <EGL/egl.h>

namespace ny
{

//A small derivate of EglSurface that holds a reference to the WindowContext it is associated with.
//If the WindowContext is not shown, the EglContext does not swapBuffers on apply()
class WaylandEglSurface : public EglSurface
{
public:
	using EglSurface::EglSurface;
	WaylandWindowContext* waylandWC_;

public:
	bool apply(std::error_code& ec) const override
	{
		if(!waylandWC_->shown()) return true;
		return EglSurface::apply(ec);
	}
};

//WaylandEglWindowContext
WaylandEglWindowContext::WaylandEglWindowContext(WaylandAppContext& ac, const EglSetup& setup,
	const WaylandWindowSettings& ws) : WaylandWindowContext(ac, ws)
{
    wlEglWindow_ = wl_egl_window_create(&wlSurface(), ws.size.x, ws.size.y);
    if(!wlEglWindow_) throw std::runtime_error("ny::WaylandEglWC: wl_egl_window_create failed");

	auto eglDisplay = setup.eglDisplay();
	auto eglnwindow = static_cast<void*>(wlEglWindow_);
	surface_ = std::make_unique<EglSurface>(eglDisplay, eglnwindow, ws.gl.config, setup);

	//store surface if requested so
	if(ws.gl.storeSurface) *ws.gl.storeSurface = surface_.get();
}

WaylandEglWindowContext::~WaylandEglWindowContext()
{
	surface_.reset();
	if(wlEglWindow_) wl_egl_window_destroy(wlEglWindow_);
}

void WaylandEglWindowContext::size(const nytl::Vec2ui& newSize)
{
	WaylandWindowContext::size(newSize);
    wl_egl_window_resize(wlEglWindow_, newSize.x, newSize.y, 0, 0);
}

bool WaylandEglWindowContext::surface(Surface& surface)
{
	surface.type = SurfaceType::gl;
	surface.gl = surface_.get();
	return true;
}

void WaylandEglWindowContext::configureEvent(nytl::Vec2ui size, WindowEdges edges)
{
	WaylandWindowContext::configureEvent(size, edges);
	wl_egl_window_resize(wlEglWindow_, size.x, size.y, 0, 0);
}

}

