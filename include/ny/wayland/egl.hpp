#pragma once

#include <ny/wayland/include.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/common/egl.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/vec.hpp>

struct wl_egl_window;

namespace ny
{

///Egl WindowContext implementation for wayland.
class WaylandEglWindowContext: public WaylandWindowContext
{
public:
    WaylandEglWindowContext(WaylandAppContext&, const EglSetup&, const WaylandWindowSettings&);
    virtual ~WaylandEglWindowContext();

	void size(const Vec2ui& newSize) override;
	bool surface(Surface& surface) override;
	bool drawIntegration(WaylandDrawIntegration*) override { return false; }

	void configureEvent(nytl::Vec2ui size, WindowEdges) override;

    wl_egl_window& wlEglWindow() const { return *wlEglWindow_; };
	EglSurface& surface() const { return *surface_; }

protected:
    wl_egl_window* wlEglWindow_ {};
	std::unique_ptr<EglSurface> surface_;
};

}
