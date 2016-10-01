#pragma once

struct xdg_shell_listener;
struct xdg_surface_listener;
struct xdg_popup_listener;

struct wl_shell_surface_listener;
struct wl_callback_listener;
struct wl_registry_listener;
struct wl_shm_listener;
struct wl_seat_listener;
struct wl_pointer_listener;
struct wl_keyboard_listener;
struct wl_callback_listener;
struct wl_data_source_listener;
struct wl_data_offer_listener;
struct wl_data_device_listener;

namespace ny
{

namespace wayland
{

extern const wl_shell_surface_listener shellSurfaceListener;
extern const wl_callback_listener frameListener;
extern const wl_registry_listener globalRegistryListener;
extern const wl_shm_listener shmListener;
extern const wl_seat_listener seatListener;
extern const wl_pointer_listener pointerListener;
extern const wl_keyboard_listener keyboardListener;
extern const wl_data_source_listener dataSourceListener;
extern const wl_data_offer_listener dataOfferListener;
extern const wl_data_device_listener dataDeviceListener;

extern const xdg_popup_listener xdgPopupListener;
extern const xdg_shell_listener xdgShellListener;
extern const xdg_surface_listener xdgSurfaceListener;

}

}
