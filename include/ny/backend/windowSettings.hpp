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
    right = 2,
    bottom = 4,
    left = 8,
    topRight = 3,
    bottomRight = 6,
    topLeft = 9,
    bottomLeft = 12,
};

using WindowEdges = nytl::Flags<WindowEdge>;
NYTL_FLAG_OPS(WindowEdge)

///Toplevel window stsyle hints.
enum class WindowHint : unsigned int
{
    close = (1L << 1), //can be closed, i.e. contains a close button/menu context
    maximize = (1L << 2), //can be maximized
    minimize = (1L << 3), //can be minimized
    resize = (1L << 4), //can be resized
    customDecorated = (1L << 5), //is customDecorated
    acceptDrop = (1L << 6), //accepts drop files
    alwaysOnTop = (1L << 7), //will always be shown on top
    showInTaskbar = (1L << 8) //will be shown in taskbar
};

using WindowHints = nytl::Flags<WindowHint>;
NYTL_FLAG_OPS(WindowHint)

///Typesafe enums that can be used for various settings with more control than just a bool.
///- must or mustNot: if the preference cannot be fulfilled an exception will be thrown or the
///  function will simply fail
///- should or shouldNot: if the preference cannot be fulfilled a warning will be raised but the
///  function will normally continue. Useful if there is method to check afterwards if preference
///  could be fulfilled.
///- dontCare: the function will decide what to do in this case. Usually the best choice.
enum class Preference : unsigned int
{
    must,
    should,
    dontCare = 0,
    shouldNot,
    mustNot
};

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

///Typesafe enums that represent the available drawing backends.
enum class DrawType : unsigned int
{
	dontCare = 0,
	none,
	gl,
	software,
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
protected:
	union
	{
		void* pointer_ = nullptr;
		std::uint64_t uint_;
	};

public:
	NativeWindowHandle(void* pointer = nullptr) : pointer_(pointer) {}
	NativeWindowHandle(std::uint64_t uint) : uint_(uint) {}

	void* pointer() const { return pointer_; }
	std::uint64_t uint() const { return uint_; }

	operator void*() const { return pointer_; }
	operator std::uint64_t() const { return uint_; }
	operator int() const { return uint_; }
	operator unsigned int() const { return uint_; }
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

struct SoftwareDrawSettings
{
	bool doubleBuffered;
	bool antialiased;
};

struct GlDrawSettings
{
	GlContext*& storeContext;
	bool contextOnly;
	bool vsync;
};

struct VulkanDrawSettings
{
	VulkanContext*& storeContext;
	bool contextOnly;
	bool vsync;
};

struct DrawSettings
{
	DrawType drawType = DrawType::dontCare;
	union
	{
		SoftwareDrawSettings softwareSettings {};
		GlDrawSettings glSettings;
		VulkanDrawSettings vulkanSettings;
	};
};

///Settings for a Window.
///Backends usually have their own WindowSettings class derived from this one.
class WindowSettings
{
public:
    virtual ~WindowSettings() = default;

    DrawType draw = DrawType::dontCare;
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
};

}
