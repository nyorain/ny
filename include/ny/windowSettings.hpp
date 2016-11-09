// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/include.hpp>
#include <ny/cursor.hpp>
#include <ny/nativeHandle.hpp>
#include <ny/surface.hpp>

#include <nytl/flags.hpp>
#include <nytl/vec.hpp>

namespace ny
{

///Typesafe enum that specifies the edges of a window (e.g. for resizing).
///Note that e.g. (WindowEdge::top | WindowEdge::right) == (WindowEdge::topRight).
enum class WindowEdge : unsigned int
{
	unknown = 0,

	top = 1,
	bottom = 2,
	left = 4,
	right = 8,
	topLeft = 5,
	bottomLeft = 6,
	topRight = 9,
	bottomRight = 10,
};
NYTL_FLAG_OPS(WindowEdge)

///Toplevel window style hints.
///They can be used to make the backend change how the window is presented.
enum class WindowHint : unsigned int
{
	close = (1L << 1), //can be closed, i.e. contains a close button/menu context
	maximize = (1L << 2), //can be maximized
	minimize = (1L << 3), //can be minimized
	resize = (1L << 4), //can be resized
	customDecorated = (1L << 5), //is customDecorated
};
NYTL_FLAG_OPS(WindowHint)

///They can be used to determine which actions can be performed on a window.
enum class WindowCapability : unsigned int
{
	none = 0,
	size = (1L << 1),
	fullscreen = (1L << 2),
	minimize = (1L << 3),
	maximize = (1L << 4),
	position = (1L << 5),
	sizeLimits = (1L << 6)
};

NYTL_FLAG_OPS(WindowCapability)

///Typesafe enum for the current state of a toplevel window.
enum class ToplevelState : unsigned int
{
	unknown = 0,
	maximized,
	minimized,
	fullscreen,
	normal
};

///Settings for creating a GlSurface for a WindowContext.
struct GlSurfaceSettings
{
	///A pointer to store a pointer to the create GlSurface.
	///Another way for retrieving the surface is to construct a surface integration
	///object (see surface.hpp).
	GlSurface** storeSurface {};

	///The config id that the surface should be created with.
	///Can be retrieved from the GlSetup implementation which can be retrieved
	///from the AppContext implementation.
	///Note that if this value is not set (i.e. the default value is used), the surface
	///will be created using the default config (should be enough for most usages).
	GlConfigId config {};
};

///Settings for creating a Vulkan Surface for a WindowContext.
struct VulkanSurfaceSettings
{
	///The vulkan instance which should be used to create the surface.
	///Note that this instance must have the needed extensions enabled, those can be
	///queried using ny::AppContext::vulkanExtensions
	VkInstance instance {};

	///A pointer to the variable in which the created surface should be stored.
	VkSurfaceKHR* storeSurface {};
};

///Settings for creating a BufferSurface for a WindowContext.
struct BufferSurfaceSettings
{
	///A pointer to a variable in which the created BufferSurface should be stored.
	BufferSurface** storeSurface {};
};


///Settings for a Window.
///Backends usually have their own WindowSettings class derived from this one.
class WindowSettings
{
public:
	NativeHandle nativeHandle; ///< May specify an already existent native handle
	NativeHandle parent; ///< May specify the windows native parent
	ToplevelState initState = ToplevelState::normal; ///< Window state after initialization
	Vec2ui size = {800, 500}; ///< Beginning window size
	Vec2i position = {~0, ~0}; ///< Beginngin window position
	std::string title = "Some Random Window Title"; ///< The title of the window
	bool show = true; ///< Show the window direclty after initialization?
	Cursor cursor; ///< Default cursor for the whole window

	///Can be used to specify if and which context should be created for the window.
	///Specifies which union member is active.
	SurfaceType surface = SurfaceType::none;
	union
	{
		GlSurfaceSettings gl {};
		VulkanSurfaceSettings vulkan;
		BufferSurfaceSettings buffer;
	};

public:
	//Constructor Needed because of the union.
	//Destructor is virtual to allow backends to dynamic_cast the Settings to detect
	//their own derivates
	WindowSettings() : vulkan() {}
	virtual ~WindowSettings() = default;
};

}