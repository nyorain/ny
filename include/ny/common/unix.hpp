// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <string_view>

namespace ny {

// Converts between values of the CursorType enum and the matching x11 cursor names
// Returns null/CursorType::unknown when conversion is not possible.
// Note that wayland uses the same names since it uses x cursor themes.
const char* cursorToXName(CursorType);
CursorType x11NameToCursor(std::string_view name);

// Converts the given Keycode to the keycode defined in linux/input.h
// At the moment, the Keycode enum matches the linux keycodes, so that it just casts the
// Keycode to an unsigned int. Note that you should prefer this function over doing so manually.
unsigned int keyToLinux(Keycode);
Keycode linuxToKey(unsigned int keycode);

unsigned int buttonToLinux(MouseButton);
MouseButton linuxToButton(unsigned int buttoncode);

// NOTE: on OSs where eventfd isn't available we could implement this using pipes
/// Non-blocking eventfd.
class EventFD {
public:
	EventFD();
	~EventFD();

	/// Signals the eventfd. After this is called, the fd will become readable
	/// (POLLIN) and reset() will return true.
	void signal();

	/// Reads and resets the eventfd. Returns whether it was signaled.
	bool reset();

	int fd() const { return fd_; }

protected:
	int fd_ {};
};

} // namespace ny
