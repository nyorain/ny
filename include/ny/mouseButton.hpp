// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

namespace ny {

/// Contains all mouse buttons.
/// Note that there might be no support for custom buttons one some backends.
/// E.g. windows only supports 2 custom mouse buttons while linux (theoretically supports
/// way too much to list here) supports 5.
enum class MouseButton : unsigned int {
	none,
	unknown, // signals that the button is just not in this enumeration but theoretically valid

	left,
	right,
	middle,

	custom1, // used by some applications as "back"
	custom2, // used by some applications as "forward"
	custom3,
	custom4,
	custom5,
	custom6
};

/// Returns the name (usually the enum value name) of the given MouseButton.
/// Returns "" for an invalid MouseButton.
const char* mouseButtonName(MouseButton);

/// Returns a MouseButton enumeration value for its name.
/// Returns MouseButton::none for unknown names.
MouseButton mouseButtonFromName(const char* name);

} // namespace ny
