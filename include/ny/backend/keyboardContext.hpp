#pragma once

#include <ny/include.hpp>
#include <ny/base/event.hpp>
#include <ny/base/key.hpp>

#include <nytl/callback.hpp>

namespace ny
{

///Keyboard interface.
///Implemented by the different backends. Can be used to handle keycodes and their unicode
///representations correctly as well as receiving general information about the keyboard.
class KeyboardContext
{
public:
	///Returns whether the given key code is pressed.
	///This functions may be async to any received events and callbacks, e.g. if checked
	///here a key might appear press although the application has not yet received (and maybe will
	///never receive) a matching KeyEvent.
	///\exception std::logic_error for invalid keycodes.
	// virtual bool pressed(Keycode keycode) const = 0;
	virtual bool pressed(Key key) const = 0;

	///Returns the utf32 encoded char that is associated with the given Keycode.
	///Useful for serializing keycodes i.e. storing them in a keymap and backend independent manner.
	///If the keycode is invalid or cannot be converted to a unicode meaning, 0 is returned.
	///\param currentState Determines whether the keycode should be converted taking the current
	///keyboard state in account (i.e. modifiers) or if the default unicode char should be
	///returned for that keycode, i.e. the char what would be generated when not taking
	///into account any other modifiers.
	// virtual char32_t keycodeToUtf32(Keycode, bool currentState = false) const = 0;

	///Returns the utf8 encoded char that is associated with the given Keycode.
	///Useful for serializing keycodes i.e. storing them in a keymap and backend independent manner.
	///If the keycode is invalid or cannot be converted to a unicode meaning, 0 is returned.
	///\param currentState Determines whether the keycode should be converted taking the current
	///keyboard state in account (i.e. modifiers) or if the default unicode char should be
	///returned for that keycode, i.e. the char what would be generated when not taking
	///into account any other modifiers.
	// virtual std::array<char, 4> keycodeToUtf8(Keycode, bool currentState = false) const = 0;
	
	// virtual Keysym keycodeToKeysym(Keycode, bool currentState = false) const = 0;
	
	virtual std::string unicode(Key key) const = 0;

	///Returns the WindowContext that has the current keyboard focus or nullptr if there
	///is none.
	virtual WindowContext* focus() const = 0;

public:
	///Will be called every time a key status changes.
	// Callback<void(Keycode keycode, std::uint32_t utf32, bool pressed)> onKey;
	Callback<void(Key key, bool pressed)> onKey;

	///Will be called every time the keyboard focus changes.
	///Note that both parameters might be a nullptr
	///It is guaranteed that both parameters will have different values.
	Callback<void(WindowContext* prev, WindowContext* now)> onFocus;
};

namespace eventType
{
	constexpr auto key = 25u;
	constexpr auto focus = 26u;
}

///Event that will be sent everytime a key is pressed or released.
class KeyEvent : public EventBase<eventType::key, KeyEvent>
{
public:
	using EvBase::EvBase;

    bool pressed; //whether it was pressed or released
    // Keycode keycode; //the raw keycode of the pressed key
	Key key;
	std::string unicode; //utf-8 encoded, keyboard state dependent
};

///Event that will be sent everytime a WindowContext gains or loses focus.
class FocusEvent : public EventBase<eventType::focus, FocusEvent>
{
public:
	using EvBase::EvBase;
	bool focus; //whether it hast gained or lost focus
};


}
