#pragma once

#include <ny/include.hpp>
#include <ny/window/defs.hpp>

namespace ny
{

//NativeWindowHandle
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

//windowSettings
class WindowSettings
{
public:
    virtual ~WindowSettings() = default;

	NativeWindowHandle nativeHandle = nullptr;
    Preference virtualPref = Preference::dontCare;
    Preference glPref = Preference::dontCare;
    ToplevelState initState = ToplevelState::normal; //only used if window is toplevel window
    bool initShown = false; //unmapped if set to false
};

}
