// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/cursor.hpp> // ny::Cursor
#include <ny/nativeHandle.hpp> // ny::NativeHandle
#include <ny/surface.hpp> // ny::SurfaceType

#include <nytl/flags.hpp> // NYTL_FLAG_OPS
#include <nytl/vec.hpp> // nytl::Vec

#include <limits> // std::numeric_limits
#include <string> // std::string

namespace ny {

/// Typesafe enum that specifies the edges of a window (e.g. for resizing).
/// Note that e.g. (WindowEdge::top | WindowEdge::right) == (WindowEdge::topRight).
enum class WindowEdge : unsigned int {
	none = 0,
	top = 1,
	bottom = 2,
	left = 4,
	right = 8,
	topLeft = 5,
	bottomLeft = 6,
	topRight = 9,
	bottomRight = 10,
};

/// They can be used to determine which actions can be performed on a window.
/// If a WindowContext does not return a certain capability, it might not be correctly
/// implemented or not implemented at all. This is needed because not all functionality
/// is available on all backends and ny tries to allow all kinds of backends.
enum class WindowCapability : unsigned int {
	none = 0,
	size = (1L << 1),
	fullscreen = (1L << 2),
	minimize = (1L << 3),
	maximize = (1L << 4),
	position = (1L << 5),
	sizeLimits = (1L << 6),
	icon = (1L << 7),
	cursor = (1L << 8),
	title = (1L << 9),
	beginMove = (1L << 10),
	beginResize = (1L << 11),
	visibility = (1L << 12),
	customDecoration = (1L << 13),
	serverDecoration = (1L << 14)
};

// Creates the binary operations for the typesafe enum classes that
// allow to combine values into nytl::Flags objects.
NYTL_FLAG_OPS(WindowEdge)
NYTL_FLAG_OPS(WindowCapability)

/// Enumreation for the current state of a toplevel window.
enum class ToplevelState : unsigned int {
	unknown = 0,
	maximized,
	minimized,
	fullscreen,
	normal
};

/// Settings for creating a GlSurface for a WindowContext.
struct GlSurfaceSettings {
	/// The config id that the surface should be created with.
	/// Can be retrieved from the GlSetup implementation which can be retrieved
	/// from the AppContext implementation.
	/// Note that if this value is not set (i.e. the default value is used), the surface
	/// will be created using the default config (should be enough for most usages).
	GlConfigID config {};

	/// A pointer to store a pointer to the create GlSurface.
	/// Another way for retrieving the surface later on is to construct a surface
	/// object (see surface.hpp).
	/// Can be nullptr.
	GlSurface** storeSurface {};
};

/// Settings for creating a Vulkan Surface for a WindowContext.
struct VulkanSurfaceSettings {
	/// The VkInstance object which should be used to create the surface.
	/// Note that this instance must have the needed extensions enabled, those can be
	/// queried using ny::AppContext::vulkanExtensions
	VkInstance instance {};

	/// The allocation callbacks for creating and destroying the surface.
	/// They only have to be valid until the surface is created.
	VkAllocationCallbacks* allocationCallbacks {};

	/// A pointer to the variable in which the created VkSurfaceKHR should be stored.
	/// Another way for retrieving the surface later on is to construct a surface
	/// object (see surface.hpp).
	/// Can be nullptr.
	std::uintptr_t* storeSurface {};
};

/// Settings for creating a BufferSurface for a WindowContext.
struct BufferSurfaceSettings {
	///A pointer to a variable in which the created BufferSurface should be stored.
	///Another way for retrieving the surface later on is to construct a surface
	///object (see surface.hpp).
	///Can be nullptr.
	BufferSurface** storeSurface {};
};

/// Used as magical signal value for no specific postion.
/// For WindowSettings::position this means to use the default value.
constexpr nytl::Vec2i defaultPosition {
	std::numeric_limits<int>::max(),
	std::numeric_limits<int>::max()
};

/// Default position if there are no backend-specific defaults and WindowSettings::position
/// was set to defaultPosition.
constexpr nytl::Vec2i fallbackPosition {100, 100};

/// Used as magical signal value for no specific size.
/// For WindowSettings::size this means to use the default size.
constexpr nytl::Vec2ui defaultSize {
	std::numeric_limits<unsigned int>::max(),
	std::numeric_limits<unsigned int>::max()
};

/// Default size if there are no backend-specific defaults and WindowSettings::size
/// was set to defaultSize.
constexpr nytl::Vec2ui fallbackSize {800, 500};

/// Settings for a Window.
/// Backends may have their own WindowSettings class derived from this one that
/// contains additional settings.
class WindowSettings {
public:
	NativeHandle parent {}; ///< May specify the windows native parent
	ToplevelState initState = ToplevelState::normal; ///< Window state after initialization
	nytl::Vec2ui size = defaultSize; ///< Beginning window size. Must not be (0, 0)
	nytl::Vec2i position = defaultPosition; ///< Beginning window position
	std::string title = "Some Random Window Title"; ///< The title of the window
	bool show = true; ///< Show the window direclty after initialization?
	Cursor cursor {}; ///< Default cursor for the whole window
	WindowListener* listener {}; ///< first listener after initialization. Can be changed
	bool transparent = false; ///< Whether to try to make the window possibly transparent
	bool droppable = false; ///< Whether the window will handle drop events

	/// Can be used to specify if and which context should be created for the window.
	SurfaceType surface = SurfaceType::none;

	/// Only the member associated with the set SurfaceType will be respected.
	GlSurfaceSettings gl {};
	VulkanSurfaceSettings vulkan {};
	BufferSurfaceSettings buffer {};

public:
	// Constructor Needed because of the union.
	// Destructor is virtual to allow backends to dynamic_cast the Settings to detect
	// their own derivates
	WindowSettings() : gl() {}
	virtual ~WindowSettings() = default;
};

} // namespace ny
