#pragma once

#include <ny/app/keyboard.hpp>
#include <ny/app/mouse.hpp>
#include <ny/app/cursor.hpp>
#include <ny/app/data.hpp>

#include <map>

#include <X11/Xlib.h>

namespace ny
{

mouse::button x11ToButton(unsigned int id);
keyboard::key x11ToKey(unsigned int id);

int cursorToX11(cursorType c);
cursorType x11ToCursor(int xcID);

long eventTypeToX11(unsigned int evType);
long eventMapToX11(const std::map<unsigned int,bool>& evMap);

}
