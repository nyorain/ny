#pragma once

#include <ny/include.hpp>
#include <ny/base/data.hpp>

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

///Window capabilites.
///They can be used to determine which actions can be performed on a window.
enum class WindowCapability : unsigned int
{
	size = (1L << 1),
	fullscreen = (1L << 2),
	minimize = (1L << 3),
	maximize = (1L << 4),
	postition = (1L << 5),
};

using WindowCapabilities = nytl::Flags<WindowCapability>;
NYTL_FLAG_OPS(WindowCapability)

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

///Enum that represents the context types that can be created for a window.
enum class ContextType : unsigned int
{
	none = 0,
	gl,
	vulkan
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
	operator std::uintptr_t() const { return uintptr(); }

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

///Type of a natvie dialog.
enum class DialogType : unsigned int
{
	none, ///Default, no dialog type
	color, ///Returns a color
	path, ///Returns a file or directory path
	custom ///Returns some custom value
};

///DialogSettingsData
///Backends that support native dialogs can derive from this struct and offer their own
///settings.
struct DialogSettingsData : public Cloneable<DialogSettingsData> {};

struct PathDialogSettingsData : public DeriveCloneable<DialogSettingsData, PathDialogSettingsData>
{
	bool allowAll;
	DataTypes allowedTypes;
};

///DialogSettings
class DialogSettings
{
public:
	DialogType type;
	std::unique_ptr<DialogSettingsData> data;

public:
	DialogSettings() = default;
	~DialogSettings() = default;

	DialogSettings(const DialogSettings& other) : type(other.type)
	{
		if(data) data = clone(*other.data);
	}

	DialogSettings& operator=(const DialogSettings& other)
	{
		type = other.type;
		data.reset();
		if(other.data) data = clone(*other.data);
		return *this;
	}
};

struct GlContextSettings
{
	//A pointer to store a pointer to the used GlContext.
	//Note that the underlaying GlContext is only guaranteed to be existent as long as
	//the WindowContext associated with the settings exists.
	GlContext** storeContext {};

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
	WindowSettings() : gl() {}
    virtual ~WindowSettings() = default;

	NativeWindowHandle nativeHandle = nullptr;
	NativeWindowHandle parent = nullptr;
	ToplevelState initState = ToplevelState::normal;
	DialogType dialogType = DialogType::none;
	Vec2ui size = {800, 500};
	Vec2i position = {~0, ~0};
	std::string title = "Some Random Window Title";
	bool initShown = true;

	NativeWidgetType nativeWidgetType = NativeWidgetType::none;
	DialogSettings dialogSettings;

	ContextType context;
	union
	{
		GlContextSettings gl;
		VulkanSurfaceSettings vulkan;
	};
};

}
