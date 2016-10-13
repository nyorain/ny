#pragma once

#include <ny/include.hpp>
#include <ny/base/data.hpp>
#include <ny/base/cursor.hpp>

#include <nytl/clone.hpp>
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

using WindowEdges = nytl::Flags<WindowEdge>;
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

using WindowHints = nytl::Flags<WindowHint>;
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

//additional capabitlity suggestions:
// - merge customDecorated with Capability
// - change cursor
// - change icon
// - make droppable
// - beginMove
// - beginResize
// - title

using WindowCapabilities = nytl::Flags<WindowCapability>;
NYTL_FLAG_OPS(WindowCapability)

//TODO: this could be really useful for all backends
///Specifies the type of the window, i.e. the intention of its display.
///This might change how the window is presented to the user by the backend.
///Otherwise just achieve it using the DialogSettings since there are mainly only 2 WindowType
///members across all platforms?
// enum class WindowType : unsigned int
// {
// 	none =  0,
// 	toplevel,
// 	dialog,
//	//...
// };

///Defines all possible native widgets that may be implemented on the specific backends.
///Note that none of them are guaranteed to exist, some backends to not have native widgets
///at all (linux backends).
enum class NativeWidgetType : unsigned int
{
	none = 0,

    button,
    textfield,
    text,
    checkbox,
    menuBar,
    toolbar,
    progressbar,
    dialog,
	dropdown
};

///Typesafe enum for the current state of a toplevel window.
enum class ToplevelState : unsigned int
{
    unknown = 0,
    maximized,
    minimized,
    fullscreen,
    normal
};


///Class to reprsent the window handle of the underlaying window system api.
///Holds either a pointer to backend-specific type or an 64 bit unsigned int.
class NativeWindowHandle
{
public:
	using Value = std::uintptr_t;

public:
	NativeWindowHandle(void* ptr = nullptr) : value_(reinterpret_cast<Value>(ptr)) {}
	NativeWindowHandle(std::uint64_t uint) : value_(reinterpret_cast<Value>(uint)) {}

	void* pointer() const { return reinterpret_cast<void*>(value_); }
	std::uint64_t uint() const { return reinterpret_cast<std::uint64_t>(value_); }
	std::uintptr_t uintptr() const { return reinterpret_cast<std::uintptr_t>(value_); }

	operator void*() const { return pointer(); }
	operator std::uint64_t() const { return uint(); }

	template<typename T> T* asPtr() const { return reinterpret_cast<T*>(value_); }

protected:
	Value value_;
};


///Result from a dialog.
enum class DialogResult : unsigned int
{
	none, ///not finished yet
	ok, ///Dialog finished as expected
	cancel ///Dialog was canceled
};

//NOTE: at the moment, there is no documentation or support for custom in any way.
///Type of a natvie dialog.
enum class DialogType : unsigned int
{
	none = 0, ///Default, no dialog type
	color, ///return: [nytl::Vec4u8], settings: [ColorDialogSettings]
	path, ///return: [c++17 ? std::path : std::string], settings[PathDialogSettings]
	custom ///custom, backend-specific (should only be used if you know what you are doing)
};

struct PathDialogSettings
{
	bool allowAll;
	DataTypes allowedTypes;
};

struct ColorDialogSettings {};


///Enum that represents the context types that can be created for a window.
enum class ContextType : unsigned int
{
	none = 0,
	gl,
	vulkan
};

struct GlContextSettings
{
	//A pointer to store a pointer to the used GlContext.
	//Note that the underlaying GlContext is only guaranteed to be existent as long as
	//the WindowContext associated with the settings exists.
	GlContext** storeContext;

	//Whether to enable vsync for the GlContext and window.
	bool vsync = true;
};

struct VulkanSurfaceSettings
{
	///The vulkan instance which should be used to create the surface.
	VkInstance instance {};

	///A pointer to a VulkanSurfaceContext in which a pointer to the used context and the
	///created surface will be stored.
	VkSurfaceKHR* storeSurface {};
};


///Settings for a Window.
///Backends usually have their own WindowSettings class derived from this one.
class WindowSettings
{
public:
	NativeWindowHandle nativeHandle = nullptr; ///< May specify an already existent native handle
	NativeWindowHandle parent = nullptr; ///< May specify the windows native parent
	ToplevelState initState = ToplevelState::normal; ///< Window state after initialization
	Vec2ui size = {800, 500}; ///< Beginning window size
	Vec2i position = {~0, ~0}; ///< Beginngin window position
	std::string title = "Some Random Window Title"; ///< The title of the window
	bool show = true; ///< Show the window direclty after initialization?
	Cursor cursor; ///< Default cursor for the whole window
	NativeWidgetType nativeWidgetType = NativeWidgetType::none;

	DialogType dialogType = DialogType::none; ///< Should the window be a native dialog?

	///May hold an object to specify the settings of the native dialog.
	///The object any holds must match the settings type specified in the DialogType enum.
	std::any dialogSettings;

	///Can be used to specify if and which context should be created for the window.
	///Specifies which union member is active.
	ContextType context = ContextType::none;
	union
	{
		GlContextSettings gl {};
		VulkanSurfaceSettings vulkan;
	};

public:
	//Needed because of the union...
	//Destructor is virtual to allow backends to dynamic_cast the Settings to detect
	//their own derivates
	WindowSettings() : gl() {}
	virtual ~WindowSettings() = default;
};

}
