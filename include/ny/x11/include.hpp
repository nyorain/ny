// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/config.hpp>

#ifndef NY_WithX11
	#error ny was built without X11. Do not include this header file!
#endif // WithX11

// xlib/xcb forward declarations
using Display = struct _XDisplay;
using xcb_connection_t = struct xcb_connection_t;
using xcb_visualtype_t = struct xcb_visualtype_t;
using xcb_screen_t = struct xcb_screen_t;
using xcb_window_t = std::uint32_t;
using xcb_atom_t = std::uint32_t;
using xcb_timestamp_t = std::uint32_t;
using xcb_cursor_t = std::uint32_t;
using xcb_pixmap_t = std::uint32_t;

using xcb_client_message_event_t = struct xcb_client_message_event_t;
using xcb_selection_notify_event_t = struct xcb_selection_notify_event_t;
using xcb_selection_request_event_t = struct xcb_selection_request_event_t;

// NOTE: sadly, we cannot forward declare xcb_generic_event_t since it's
// an anonymous struct typedef in xcb/xcb.h. Using void* instead.

namespace ny {

class X11WindowContext;
class X11AppContext;
class X11MouseContext;
class X11KeyboardContext;
class X11DataManager;
class X11ErrorCategory;
struct X11EventData;

class GlxContext;
class GlxSetup;
class GlxSurface;

class GlxWindowContext;
class X11VulkanWindowContext;
class X11BufferWindowContext;
class X11BufferSurface;

namespace x11 {

/// Dummy delcaration to not include xcb_ewmh.h
/// The xcb_ewmh_connection_t type cannot be forward declared since it is a unnamed
/// struct typedef in the original xcb_ewmh header, which should not be included in a
/// header file.
struct EwmhConnection;
struct GenericEvent;

struct Atoms;
struct Property;

} // namespace x11
} // namespace ny
