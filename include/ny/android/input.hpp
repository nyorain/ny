// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/android/include.hpp>
#include <ny/keyboardContext.hpp>
#include <ny/mouseContext.hpp>
#include <ny/event.hpp>

#include <bitset> // std::bitset

namespace ny {

/// Android KeyboardContext implementation.
class AndroidKeyboardContext : public KeyboardContext {
public:
	AndroidKeyboardContext(AndroidAppContext& ac) : appContext_(ac) {}
	~AndroidKeyboardContext() = default;

	bool pressed(Keycode) const override;
	std::string utf8(Keycode) const override;
	WindowContext* focus() const override;
	KeyboardModifiers modifiers() const override;

	// - android specific -
	/// Proccess the given key event.
	/// Return true if was processed.
	bool process(const AInputEvent& event);

protected:
	AndroidAppContext& appContext_;
	std::bitset<255> keyStates_;
	KeyboardModifiers modifiers_;
};

/// Android MouseContext implementation.
/// Mainly based on touch events, will emulate them as left button
/// mouse events until further mouse/touch work in ny base.
class AndroidMouseContext : public MouseContext {
public:
	AndroidMouseContext(AndroidAppContext& ac) : appContext_(ac) {}
	~AndroidMouseContext() = default;

	nytl::Vec2i position() const override;
	bool pressed(MouseButton button) const override;
	WindowContext* over() const override;

	// - android specific -
	/// Proccess the given motion event.
	/// Return true if was processed.
	bool process(const AInputEvent& event);

protected:
	AndroidAppContext& appContext_;
};

/// Conerts the given android keycode (from android/keycodes.h) to
/// the associated ny keycode. Return Keycode::none for unknown keycodes.
Keycode androidToKeycode(unsigned int code);

/// Conerts the given ny keycode to the android equivalent.
/// Returns AKEYCODE_UNKNOWN for unsupported keycodes.
unsigned int keycodeToAndroid(Keycode);

/// Converts the given mask of android modifiers to their equivalent KeyboardModifiers.
KeyboardModifiers androidToModifiers(unsigned int mask);

/// Conerts the given KeyboardModifiers to their android equivalent.
unsigned int modifiersToAndroid(KeyboardModifiers mods);

/// Android EventData holding the associated AInputEvent.
/// Note that the AInputEvent is only valid during the callback.
struct AndroidEventData : public EventData {
	const AInputEvent* inputEvent {};
};

} // namespace ny
