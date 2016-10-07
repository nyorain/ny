#pragma once

#include <cstdint>
#include <array>

namespace ny
{

//1. Keyboard press (keyboard generates scancode/keycode for hardware key and sends it)
//2. Backend server receives event and only forwards it as well as the layout to the client
//3. The client has keymap information and the keycode, is now able to handle it

///The keycode class represents a backends keycode.
// class Keycode
// {
// public:
// 	Keycode() = default;
// 	virtual ~Keycode() = default;
// 
// 	///Returns the keycode 
// 	virtual std::uint32_t utf32() const = 0;
// 	virtual std::array<char, 6> utf8() const = 0;
// 	virtual std::uint64_t value() const = 0;
// 
// 	virtual bool same(const Keycode& other) const = 0;
// };

//better this?

///The Keycode struct is only a typesafe wrapper around a 64-bit unsigned integer value.
///This integer value should represent some hardware keyboard key by the backend.
///The value is completely independent from some current keyboard state, i.e. the keyboard
///key 'A' should always generate the same Keycode no matter if the shift, control or alt
///modifier is currently active. 
///Note that one should never try give the keycode any meaning since e.g. the keycode
///generated from pressing the same hardware key may differ on different backends and
///the meaning associated with a keycode is dependent on the used keymap.
///This is useful where a utf representation (i.e. the meaning for the current state) 
///of a key contains unneeded information and the application intends to deal with the
///raw hardware keys (which could be e.g. the case for dynamic game controls).
class Keycode 
{ 
public:
	///Represents an invalid keycode that has no associated hardware key.
	static Keycode none;
	using Value = std::uint64_t;

public:
	Value value {}; 

public:
	constexpr Keycode() = default;
	constexpr Keycode(Value v) : value(v) {}

	constexpr operator Value() const { return value; }
};

constexpr bool operator==(const Keycode& a, const Keycode& b) { return a.value == b.value; }
constexpr bool operator!=(const Keycode& a, const Keycode& b) { return a.value != b.value; }
constexpr bool operator<=(const Keycode& a, const Keycode& b) { return a.value <= b.value; }
constexpr bool operator<(const Keycode& a, const Keycode& b) { return a.value < b.value; }
constexpr bool operator>=(const Keycode& a, const Keycode& b) { return a.value >= b.value; }
constexpr bool operator>(const Keycode& a, const Keycode& b) { return a.value > b.value; }

//appContext. Better KeyboardContext:

///Returns a Keycode that could generate the given utf32 encoded unicode char.
///If there is no such Keycode, Keycode::none is returned.
virtual Keycode keycodeFromUtf32(char32_t utf32) = 0;

///Returns a Keycode that could generate the given utf8 encoded unicode char.
///If there is no such Keycode, Keycode::none is returned.
virtual Keycode keycodeFromUtf8(const std::array<char, 4>& utf8) = 0;

///Returns the utf32 encoded char that is associated with the given Keycode. Usually this
///represents the char that would be generated if the key would pressed without any other
///active modifier.
///Useful for serializing keycodes i.e. storing them in a keymap and backend independent manner.
///If the keycode is invalid or cannot be converted to a unicode meaning, 0 is returned.
virtual char32_t keycodeToUtf32(Keycode) = 0;

///Returns the utf8 encoded char that is associated with the given Keycode.Usually this
///represents the char that would be generated if the key would pressed without any other
///active modifier.
///Useful for serializing keycodes i.e. storing them in a keymap and backend independent manner.
///If the keycode is invalid or cannot be converted to a unicode meaning, 0 is returned.
virtual std::array<char, 4> keycodeToUtf8(Keycode) = 0;

///Returns a name that can be associated with the given keycode.
///This name should be used for serializing/storing/comparing the hardware key rather
///than the raw Keycode. The returned name should be the same for all backends and
///also keymap-aware.
///If the Keycode is invalid or has no name, an empty string is returned.
virtual std::string keycodeName(Keycode) = 0;

///Constrcuts the backends Keycode for the given Keycode name.
///If the name is not recognized or invalid, Keycode::none is returned.
virtual Keycode keycodeFromName(const std::string& name) = 0;

//forget this...
enum class Key
{
    none = 0,

    a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    n0, n1, n2, n3, n4, n5, n6, n7, n8, n9,
	numpad0, numpad1, numpad2, numpad3, numpad4, numpad5, numpad6, numpad7, numpad8, numpad9,

    f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19,
	f20, f21, f22, f23, f24,

    play, stop, pause, next, previous, 
	escape, sharp, tab,
	leftctrl, rightctrl, leftsuper, rightsuper, leftshift, rightshift, leftalt, rightalt, capsLock,
    space, enter, backspace, del, end, insert, pageUp, pageDown, home, back, 
	left, up, down, right, volumeup, volumedown,

	question, exclamation, numbersign, percent, ampersand, apostrophe, parenleft, pranright,
	aterisk, plus, comma, minus, period, slash, colon, semicolon, less, equal, greater, at,
	backslash, brecketleft, bracketright, braceleft, braceright,  
	cent, dollars, yen, euro
};

}
