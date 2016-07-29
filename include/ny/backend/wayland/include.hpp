#pragma once

#include <ny/include.hpp>
#include <cstdint>

#ifndef NY_WithWayland
	#error ny was built without wayland. Do not include this header file!
#endif //WithX11

//wayland forward decls
struct xdg_surface;
struct xdg_popup;
struct xdg_shell;

struct wl_buffer;
struct wl_callback;
struct wl_compositor;
struct wl_data_device;
struct wl_data_device_manager;
struct wl_data_offer;
struct wl_data_source;
struct wl_display;
struct wl_keyboard;
struct wl_output;
struct wl_pointer;
struct wl_region;
struct wl_registry;
struct wl_seat;
struct wl_shell;
struct wl_shell_surface;
struct wl_shm;
struct wl_shm_pool;
struct wl_subcompositor;
struct wl_subsurface;
struct wl_surface;
struct wl_touch;
struct wl_array;
struct wl_cursor;
struct wl_cursor_theme;

typedef std::int32_t wl_fixed_t;

namespace ny
{
	class WaylandBackend;
	class WaylandWindowContext;
	class WaylandAppContext;
	class WaylandWindowSettings;

	namespace wayland
	{
		class ShmBuffer;
		class ServerCallback;
		class Output;

		enum class SurfaceRole : unsigned char;
	}

}

