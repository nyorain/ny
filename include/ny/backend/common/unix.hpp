#pragma once

#include <ny/include.hpp>
#include <nytl/stringParam.hpp>

namespace ny
{

//Converts between values of the CursorType enum and the matching x11 cursor names
//Note that wayland uses the same names since it uses x cursor themes.
const char* cursorToXName(CursorType);
CursorType x11NameToCursor(const nytl::StringParam& name);

//Converts the given Keycode to the keycode defined in linux/input.h
//At the moment, the Keycode enum matches the linux keycodes, so that it just casts the
//Keycode to an unsigned int. Note that you should prefer this function over doing so manually.
unsigned int keyToLinux(Keycode);
Keycode linuxToKey(unsigned int keycode);

unsigned int buttonToLinux(MouseButton);
MouseButton linuxToButton(unsigned int buttoncode);


}
