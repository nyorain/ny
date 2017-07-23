// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/backend.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/appContext.hpp>

#include <wayland-client-core.h>

namespace ny {

void WaylandBackend::initialize()
{
	static WaylandBackend instance_;
}

bool WaylandBackend::available() const
{
	wl_display* dpy = wl_display_connect(nullptr);
	if(!dpy) return false;
	wl_display_disconnect(dpy);
	return true;
}

AppContextPtr WaylandBackend::createAppContext()
{
	return std::make_unique<WaylandAppContext>();
}

bool WaylandBackend::gl() const
{
	return builtWithGl();
}

bool WaylandBackend::vulkan() const
{
	return builtWithVulkan();
}

} // namespace ny
