#pragma once

#include <ny/include.hpp>
#include <nytl/vec.hpp>
#include <nytl/enumOps.hpp>

#include <cstdint>
#include <bitset>

namespace ny
{

///Toplevel window stsyle hints.
enum class WindowHints : unsigned int
{
    close = (1L << 1),
    maximize = (1L << 2),
    minimize = (1L << 3),
    resize = (1L << 4),
    customDecorated = (1L << 5),
    acceptDrop = (1L << 6),
    alwaysOnTop = (1L << 7),
    showInTaskbar = (1L << 8) 
};

///Typesafe enum that specifies the edges of a window (e.g. for resizing).
///Note that e.g. (WindowEdge::top | WindowEdge::right) == (WindowEdge::topRight).
enum class WindowEdges : unsigned char
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

///Typesafe enums that can be used for various settings with more control than just a bool.
///- must or mustNot: if the preference cannot be fulfilled an exception will be thrown or the
///  function will simply fail
///- should or shouldNot: if the preference cannot be fulfilled a warning will be raised but the
///  function will normally continue. Useful if there is method to check afterwards if preference
///  could be fulfilled.
///- dontCare: the function will decide what to do in this case. Usually the best choice.
enum class Preference : unsigned char
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
enum class NativeWidgetType : unsigned char
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
enum class DrawType : unsigned char
{
	dontCare = 0,
	none,
	gl,
	software,
	vulkan
};

///Typesafe enum for the current state of a toplevel window.
enum class ToplevelState : unsigned char
{
    unknown = 0,

    maximized,
    minimized,
    fullscreen,
    normal
};

///Class to reprsent the window handle of the underlaying window system api.
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

///Settings for a Window.
class WindowSettings
{
public:
    virtual ~WindowSettings() = default;

    DrawType draw = DrawType::dontCare;
	std::bitset<64> events = {1};
	NativeWindowHandle nativeHandle = nullptr;
	NativeWindowHandle parent = nullptr;
	ToplevelState initState = ToplevelState::normal;
	NativeWidgetType nativeWidgetType = NativeWidgetType::none;
	Vec2ui size = {800, 500};
	Vec2i position = {~0, ~0};
	std::string title = "Some Random Window Title";
	bool initShown = true;
};

}

NYTL_ENABLE_ENUM_OPS(ny::WindowHints)
NYTL_ENABLE_ENUM_OPS(ny::WindowEdges)
