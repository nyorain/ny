#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/gl/egl.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/vec.hpp>

struct wl_egl_window;

namespace ny
{

// class waylandEGLAppContext 
// {
// public:
//     waylandEGLAppContext(waylandAppContext* ac);
//     ~waylandEGLAppContext();
// };


///Egl WindowContext implementation for wayland.
class WaylandEglWindowContext: public WaylandWindowContext
{
public:
    WaylandEglWindowContext(const WaylandWindowContext& wc, const WaylandWindowSettings& settings);
    virtual ~WaylandEglWindowContext();

    wl_egl_window& wlEglWindow() const { return *wlEglWindow_; };

protected:
    wl_egl_window* wlEglWindow_ = nullptr;
	EglContext eglContext_;
};


}
