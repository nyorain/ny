#pragma once

#include "app/keyboard.hpp"
#include "app/mouse.hpp"
#include "app/cursor.hpp"
#include "app/dnd.hpp"

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
