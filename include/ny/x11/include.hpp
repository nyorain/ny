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
using xcb_window_t = uint32_t;
using xcb_atom_t = uint32_t;

namespace ny {

class X11WindowContext;
class X11AppContext;
class X11MouseContext;
class X11KeyboardContext;
class X11DataManager;
class X11ErrorCategory;

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
