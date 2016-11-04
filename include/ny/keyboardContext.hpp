#pragma once

#include <ny/include.hpp>
#include <ny/event.hpp>

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
	///This functions may be async to any received events and callbacks e.g. if checked
	///here a key might appear press although the application has not yet received (and maybe will
	///never receive) a matching KeyEvent.
	///\exception std::logic_error for invalid keycodes.
	virtual bool pressed(Keycode keycode) const = 0;

	///Converts the given Keycode to utf8 encoded characters.
	///If the Keycode cannot be represented using unicode (e.g. leftshift or escape) an
	///empty string will be returned.
	///Usually the returned string should only one utf8 encoded unicode value but in
	///some cases (e.g. character composition, added dead keys) it may hold more characters.
	///Remember that std::string[0] does NOT return the first unicode character of
	///a string but the first 8-bit char.
	///\param currentState Whether the returned unicode values should be dependent
	///on the current keyboard state (e.g. modifiers). If this is false, the
	///default unicode value for the given Keycode will be returned.
	///\exception std::logic_error for invalid keycodes.
	virtual std::string utf8(Keycode, bool currentState = false) const = 0;

	///Returns the WindowContext that has the current keyboard focus or nullptr if there
	///is none.
	///This function may be async to any received events and callbacks i.e. the WindowContext
	///might be returned as the focused one here although it has not yet handled the focus
	///event.
	virtual WindowContext* focus() const = 0;

public:
	///Will be called every time a key status changes.
	// Callback<void(Keycode keycode, std::uint32_t utf32, bool pressed)> onKey;
	nytl::Callback<void(const KeyboardContext&, Keycode, std::string utf8, bool pressed)> onKey;

	///Will be called every time the keyboard focus changes.
	///Note that both parameters might be a nullptr
	///It is guaranteed that both parameters will have different values.
	nytl::Callback<void(const KeyboardContext&, WindowContext* prev, WindowContext* now)> onFocus;
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
    Keycode keycode; //the raw keycode of the pressed key
	std::string unicode; //utf-8 encoded, keyboard state dependent
};

///Event that will be sent everytime a WindowContext gains or loses focus.
class FocusEvent : public EventBase<eventType::focus, FocusEvent>
{
public:
	using EvBase::EvBase;
	bool focus; //whether it gained focus
};


}
