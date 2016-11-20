// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

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

	void size(nytl::Vec2ui size) override;
	Surface surface() override;

    wl_egl_window& wlEglWindow() const { return *wlEglWindow_; };
	EglSurface& surface() const { return *surface_; }

protected:
    wl_egl_window* wlEglWindow_ {};
	std::unique_ptr<EglSurface> surface_;
};

}
