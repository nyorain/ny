// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/include.hpp>
#include <ny/keyboardContext.hpp>
#include <bitset> // std::bitset

namespace ny {

/// Android KeyboardContext implementation.
class AndroidKeyboardContext : public KeyboardContext {
public:
	AndroidKeyboardContext(AndroidAppContext&);
	~AndroidKeyboardContext();

	bool pressed(Keycode key) const override;
	std::string utf8(Keycode) const override;
	WindowContext* focus() const override;
	KeyboardModifiers modifiers() const override;

	// - android specific -
	/// Proccess the given android event.
	/// Return true if was processed.
	bool process(const AInputEvent& event);

protected:
	AndroidAppContext& appContext_;
	std::bitset<255> keyStates_;
};

/// Conerts the given android keycode (from android/keycodes.h) to
/// the associated ny keycode. Return Keycode::none for unknown keycodes.
Keycode androidToKeycode(unsigned int code);

/// Conerts the given ny keycode to the android equivalent.
/// Returns AKEYCODE_UNKNOWN for unsupported keycodes.
unsigned int keycodeToAndroid(Keycode);

} // namespace ny
