// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>

#include <nytl/vec.hpp> // nytl::Vec
#include <nytl/callback.hpp> // nytl::Callback

namespace ny {

/// MouseContext interface, implemented by a backend.
class MouseContext {
public:
	/// Returns the newest mouse position that can be queried for the current window under
	/// the pointer.
	/// Note that if the pointer is not over a window, the result of this function is undefined.
	virtual nytl::Vec2i position() const = 0;

	/// Returns whether the given button is pressed.
	/// This functions may be async to any received events and callbacks.
	virtual bool pressed(MouseButton button) const = 0;

	/// Returns the WindowContext over that the pointer is located, or nullptr if there is none.
	virtual WindowContext* over() const = 0;

public:
	/// Will be called everytime a mouse button is clicked or released.
	nytl::Callback<void(MouseContext&, MouseButton, bool pressed)> onButton;

	/// Will be called everytime the mouse moves.
	nytl::Callback<void(MouseContext&, nytl::Vec2i pos, nytl::Vec2i delta)> onMove;

	/// Will be called everytime the pointer focus changes.
	/// Note that both parameters might be a nullptr
	nytl::Callback<void(MouseContext&, WindowContext* prev, WindowContext* now)> onFocus;

	/// Will be called everytime the mousewheel is rotated.
	/// A value >0 means that the wheel was rotated forwards, a value < 0 backwards.
	nytl::Callback<void(MouseContext&, float value)> onWheel;
};

} // namespace ny
