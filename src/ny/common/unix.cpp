// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/common/unix.hpp>
#include <ny/mouseContext.hpp>
#include <ny/mouseButton.hpp>
#include <ny/cursor.hpp>
#include <dlg/dlg.hpp>
#include <cstring>

namespace ny {

// https://www.freedesktop.org/wiki/Specifications/cursor-spec/
// https://github.com/GNOME/gtk/blob/master/gdk/wayland/gdkcursor-wayland.c
// https://github.com/wayland-project/weston/blob/master/clients/window.c#L1220
// TODO: we could return a vector of names per cursor type and then
// (backend impl) we have fallbacks to load if the first one doesn't work

constexpr struct {
	CursorType cursor;
	const char* name;
} mapping[] = {
	{CursorType::leftPtr, "left_ptr"},
	{CursorType::load, "watch"},
	{CursorType::loadPtr, "left_ptr_watch"},
	{CursorType::rightPtr, "right_ptr"},
	{CursorType::hand, "pointer"},
	{CursorType::grab, "grab"},
	{CursorType::crosshair, "cross"},
	{CursorType::help, "question_arrow"},
	{CursorType::beam, "xterm"},
	{CursorType::forbidden, "crossed_circle"},
	{CursorType::size, "bottom_left_corner"},
	{CursorType::sizeBottom, "bottom_side"},
	{CursorType::sizeBottomLeft, "bottom_left_corner"},
	{CursorType::sizeBottomRight, "bottom_right_corner"},
	{CursorType::sizeTop, "top_side"},
	{CursorType::sizeTopLeft, "top_left_corner"},
	{CursorType::sizeTopRight, "top_right_corner"},
	{CursorType::sizeLeft, "left_side"},
	{CursorType::sizeRight, "right_side"},
};

const char* cursorToXName(CursorType cursor) {
	for(auto& m : mapping) {
		if(m.cursor == cursor) {
			return m.name;
		}
	}

	return nullptr;
}

CursorType xNameToCursor(std::string_view name) {
	for(auto& m : mapping) {
		if(!std::strncmp(m.name, name.data(), name.size())) {
			return m.cursor;
		}
	}

	return CursorType::unknown;
}

unsigned int keyToLinux(Keycode keycode) {
	return static_cast<unsigned int>(keycode);
}

Keycode linuxToKey(unsigned int keycode) {
	return static_cast<Keycode>(keycode);
}

unsigned int buttonToLinux(MouseButton button) {
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

MouseButton linuxToButton(unsigned int buttoncode) {
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
