// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/common/unix.hpp>
#include <ny/mouseContext.hpp>
#include <ny/mouseButton.hpp>
#include <ny/cursor.hpp>

namespace ny {

const char* cursorToXName(CursorType cursor)
{
	switch(cursor) {
		case CursorType::leftPtr: return "left_ptr";
		case CursorType::sizeBottom: return "bottom_side";
		case CursorType::sizeBottomLeft: return "bottom_left_corner";
		case CursorType::sizeBottomRight: return "bottom_right_corner";
		case CursorType::sizeTop: return "top_side";
		case CursorType::sizeTopLeft: return "top_left_corner";
		case CursorType::sizeTopRight: return "top_right_corner";
		case CursorType::sizeLeft: return "left_side";
		case CursorType::sizeRight: return "right_side";
		case CursorType::hand: return "fleur";
		case CursorType::grab: return "grabbing";
		default: return nullptr;
	}
}

CursorType xNameToCursor(std::string_view name)
{
	if(name == "left_ptr") return CursorType::leftPtr;
	if(name == "right_ptr") return CursorType::rightPtr;
	if(name == "bottom_side") return CursorType::sizeBottom;
	if(name == "left_side") return CursorType::sizeLeft;
	if(name == "right_side") return CursorType::sizeRight;
	if(name == "top_side") return CursorType::sizeTop;
	if(name == "top_side") return CursorType::sizeTop;
	if(name == "top_left_corner") return CursorType::sizeTopLeft;
	if(name == "top_right_corner") return CursorType::sizeTopRight;
	if(name == "bottom_right_corner") return CursorType::sizeBottomRight;
	if(name == "bottom_left_corner") return CursorType::sizeBottomLeft;
	if(name == "fleur") return CursorType::hand;
	if(name == "grabbing") return CursorType::grab;
	return CursorType::unknown;
}

unsigned int keyToLinux(Keycode keycode)
{
	return static_cast<unsigned int>(keycode);
}

Keycode linuxToKey(unsigned int keycode)
{
	return static_cast<Keycode>(keycode);
}

unsigned int buttonToLinux(MouseButton button)
{
	switch(button) {
		case MouseButton::left: return 0x110;
		case MouseButton::right: return 0x111;
		case MouseButton::middle: return 0x112;

		case MouseButton::custom1: return 0x113;
		case MouseButton::custom2: return 0x114;
		case MouseButton::custom3: return 0x115;
		case MouseButton::custom4: return 0x116;
		case MouseButton::custom5: return 0x117;

		default: return 0;
	}
}

MouseButton linuxToButton(unsigned int buttoncode)
{
	switch(buttoncode) {
		case 0x110: return MouseButton::left;
		case 0x111: return MouseButton::right;
		case 0x112: return MouseButton::middle;
		case 0x113: return MouseButton::custom1;
		case 0x114: return MouseButton::custom2;
		case 0x115: return MouseButton::custom3;
		case 0x116: return MouseButton::custom4;
		case 0x117: return MouseButton::custom5;
		default: return MouseButton::unknown;
	}
}

} // namespace ny
