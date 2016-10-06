#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/common/egl.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/vec.hpp>

struct wl_egl_window;

namespace ny
{

//TODO: support for multiple configs, impl config querying
///RAII wrapper around EGLDisplay and EglContextGuard.
class WaylandEglDisplay
{
public:
    WaylandEglDisplay(WaylandAppContext&);
    ~WaylandEglDisplay();

	EGLDisplay eglDisplay() const { return eglDisplay_; }
	EGLContext eglContext() const { return eglContext_; }
	EGLConfig eglConfig() const { return eglConfig_; }

protected:
	EGLDisplay eglDisplay_;
	EGLContext eglContext_;
	EGLConfig eglConfig_;
};


///Egl WindowContext implementation for wayland.
class WaylandEglWindowContext: public WaylandWindowContext
{
public:
    WaylandEglWindowContext(WaylandAppContext&, const WaylandWindowSettings&);
    virtual ~WaylandEglWindowContext();

	void size(const Vec2ui& newSize) override;
	bool surface(Surface& surface) override;
	bool drawIntegration(WaylandDrawIntegration*) override { return false; }

    wl_egl_window& wlEglWindow() const { return *wlEglWindow_; };
	EGLSurface eglSurface() const { return eglSurface_; }

	EglContext& context() const { return *context_; }

protected:
    wl_egl_window* wlEglWindow_ {};
	EGLSurface eglSurface_ {};
	std::unique_ptr<EglContext> context_;
};

}
