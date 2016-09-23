#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/common/egl.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/vec.hpp>

struct wl_egl_window;
namespace evg { class GlDrawContext; }

namespace ny
{

class WaylandEGLAppContext 
{
public:
    WaylandEGLAppContext(WaylandAppContext& ac);
    ~WaylandEGLAppContext();
};


///Egl WindowContext implementation for wayland.
class WaylandEglWindowContext: public WaylandWindowContext
{
public:
    WaylandEglWindowContext(WaylandAppContext& wc, const WaylandWindowSettings& settings);
    virtual ~WaylandEglWindowContext();

	void size(const Vec2ui& size) override;

    wl_egl_window& wlEglWindow() const { return *wlEglWindow_; };

protected:
    wl_egl_window* wlEglWindow_ = nullptr;
	std::unique_ptr<EglContext> eglContext_;
	std::unique_ptr<evg::GlDrawContext> drawContext_;
};

}
