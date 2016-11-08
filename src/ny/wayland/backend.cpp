#include <ny/wayland/backend.hpp>

#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/appContext.hpp>

#include <wayland-client-core.h>

namespace ny
{

WaylandBackend WaylandBackend::instance_;

WaylandBackend::WaylandBackend()
{
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

}
