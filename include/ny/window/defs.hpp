#pragma once

#include <ny/include.hpp>
#include <cstdint>

namespace ny
{

enum WindowHint
{
    close = (1L << 2),
    maximize = (1L << 3),
    minimize = (1L << 4),
    resize = (1L << 5),
    customDecorated = (1L << 7),
    acceptDrop = (1L << 10),
    alwaysOnTop = (1L << 11),
    showInTaskbar = (1L << 12)
};

///Typesafe enum that specifies the edges of a window (e.g. for resizing).
///Note that e.g. (WindowEdge::top | WindowEdge::right) == (WindowEdge::topRight). You have
///to include <nytl/enumOps.hpp> to make those operations with typesafe enums work.
enum class WindowEdge : unsigned char
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
    dontCare,
    shouldNot,
    mustNot
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

	NativeWindowHandle nativeHandle {};
    Preference virtualPref = Preference::dontCare;
    Preference glPref = Preference::dontCare;
    ToplevelState initState = ToplevelState::normal; //only used if window is toplevel window
    bool initShown = true; //unmapped if set to false
};

}
