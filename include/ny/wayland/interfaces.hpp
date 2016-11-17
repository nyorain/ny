#pragma once

struct xdg_shell_listener;
struct xdg_surface_listener;
struct xdg_popup_listener;

struct wl_shell_surface_listener;
struct wl_callback_listener;
struct wl_registry_listener;
struct wl_shm_listener;
struct wl_seat_listener;

namespace ny
{

namespace wayland
{

extern const wl_shell_surface_listener shellSurfaceListener;
extern const wl_callback_listener frameListener;

extern const xdg_popup_listener xdgPopupListener;
extern const xdg_surface_listener xdgSurfaceListener;

}

}
