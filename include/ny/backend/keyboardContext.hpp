#pragma once

#include <ny/include.hpp>
#include <ny/base/event.hpp>

#include <nytl/callback.hpp>

namespace ny
{

//TODO: something about modifiers for manual parsing?
///Contains a list of all common keyboard symbols.
enum class Key : unsigned int
{
    none = 0,

    a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    n0, n1, n2, n3, n4, n5, n6, n7, n8, n9,
	numpad0, numpad1, numpad2, numpad3, numpad4, numpad5, numpad6, numpad7, numpad8, numpad9,

    f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19,
	f20, f21, f22, f23, f24,

    play, stop, pause, next, previous, 
	escape, comma, semicolon, dot, sharp, plus, minus, tab,
	leftctrl, rightctrl, leftsuper, rightsuper, leftshift, rightshift, leftalt, rightalt, capsLock,
    space, enter, backspace, del, end, insert, pageUp, pageDown, home, back, 
	left, up, down, right, volumeup, volumedown,

	backslash, brecketleft, bracketright, braceleft, braceright,
	cent, dollars, yen, euro
};


///Keyboard interface.
class KeyboardContext
{
public:
	///Returns whether the given key code is pressed.
	///This functions may be async to any received events and callbacks.
	virtual bool pressed(Key key) const = 0;

	///Parses the given key to a utf8-encoded unicode string depending on the
	///keyboards current state.
	///This functions may be async to any received events and callbacks.
	///If the key cannot be parsed to any text (like e.g. f5, play, left or rightalt), an
	///empty string should be returned.
	virtual std::string unicode(Key key) const = 0;

	///Returns the WindowContext that has the current keyboard focus or nullptr if there
	///is none.
	virtual WindowContext* focus() const = 0;

	///Will be called every time a key status changes.
	Callback<void(Key key, bool pressed)> onKey;

	///Will be called every time the keyboard focus changes.
	///Note that both parameters might be a nullptr
	///It is guaranteed that both parameters will have different values.
	Callback<void(WindowContext* prev, WindowContext* now)> onFocus;
};

//Events
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
    Key key;
	std::string unicode; ///utf-8 encoded.
};

///Event that will be sent everytime an EventHandler gains or loses focus.
class FocusEvent : public EventBase<eventType::focus, FocusEvent>
{
public:
	using EvBase::EvBase;
	bool focus; //whether it hast gained or lost focus
};


}
