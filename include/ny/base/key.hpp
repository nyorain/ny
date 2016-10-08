#pragma once

#include <cstdint>
#include <array>

namespace ny
{

///Enumeration that holds all special key constants that cannot (or not easily) be represented
///using unicode.
enum class Key : std::uint32_t
{
    none = 0,
	unicode = 1,
	unkown = 3,

	numpad0, numpad1, numpad2, numpad3, numpad4, numpad5, numpad6, numpad7, numpad8, numpad9,

    f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19,
	f20, f21, f22, f23, f24,

    play, stop, pause, next, previous, 
	escape, leftctrl, rightctrl, leftsuper, rightsuper, leftshift, rightshift, leftalt, rightalt, 
	capsLock, space, enter, backspace, del, end, insert, pageUp, pageDown, home, back, 
	left, up, down, right, volumeup, volumedown,
};

///The Keycode struct is only a typesafe wrapper around a 64-bit unsigned integer value.
///This integer value should represent some hardware keyboard key by the backend.
///The value is completely independent from some current keyboard state, i.e. the keyboard
///key 'A' should always generate the same Keycode no matter if the shift, control or alt
///modifier is currently active. 
///Note that one should never try give the keycode any meaning since e.g. the keycode
///generated from pressing the same hardware key may differ on different backends and
///the meaning associated with a keycode is dependent on the used keymap.
///This is only useful where a utf representation (i.e. the meaning for the current state) 
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

///Basically just a utf32 encoded char or a special value of the Key enumeration.
///Note that the first idea was to just represent every keysym as their raw utf32 equivalent
///but the problem is that keyboards usually have keys that have no real unicode equivalent 
///(at least not for their meaning, maybe there exists one for their symbol).
///Example keys would be media keys (play, pause, volume), directional arrow keys or
///shift/alt/super modifiers.
///Furthermore it can be inconvinient to always use the raw unicode values in code e.g. when
///explicitly checking for certain chars like escape.
class Keysym
{
public:
	Keysym(Key xkey = Key::none, char32_t xutf32 = 0) : key_(xkey), utf32_(xutf32) {};
	Keysym(std::uint64_t serialized);

	///If the keysym has a representation in the Key enum it will be returned.
	///If the keysym has no such representation, but it displayable as a unicode code,
	///Key::unicode will be returned.
	///For an empty Keysym (e.g. default constructed), Key::none will be returned.
	///If the Keysym represents a valid keysym that can neither represented using unicode, nor
	///has a entry in the Key enumeration, Key::unkown will be returned.
	///Note that some keys (like e.g. escape) can be displayed as a unicode char but have
	///a Key enum representation for convinience.
	Key type() const { return key_; }

	///If the keysym can be represented using unicode, this function returns its utf32-encoded
	///unicode char.
	///If the keysym holds no keysym or it cannot be represented as a unicode value, 0 is returned.
	///Note that this might overlap with the NUL character ('\0') so to actually test if
	///it can be represented as unicode, the return value of the key() function should be checked.
	char32_t utf32() const { return utf32_; }

	///\{
	///Changes the value of this keysym to the given utf8 encoded unicode value.
	void utf8(const std::string& value, Key = Key::unicode);
	void utf8(const char* value, Key = Key::unicode);
	void utf8(const std::array<char, 4>& value, Key = Key::unicode);
	///\}

	///Changes the value of this Keysym to the given utf32 encoded unicode value.
	void utf32(char32_t value, Key = Key::unicode);

	///Sets the value of this Keysym to the given keytype.
	///Note that this will reset the utf32 representation.
	///If the special Key type could also represented using unicode, prefer to call one of
	///the utf functions to set the new Key and unicode value.
	///\exception std::logic_error if the given Key is Key::unicode.
	void type(Key);

	///If the keysym can be represented using unicode, this function returns its utf8-encoded
	///unicode char.
	///If the keysym holds no keysym or it cannot be represented as a unicode value, an array
	///filled with zeroes is returned.
	///Note that this might overlap with the NUL character ('\0') so to actually test if
	///it can be represented as unicode, the return value of the key() function should be checked.
	std::array<char, 4> utf8() const;

	///Serializes the keysym to a single 64-bit unsigned integer that can be e.g. used to store
	///the keysym in a file. If read, simply use the Keysym constructor that just takes
	///a 64-bit unsigned integer as parameter.
	std::uint64_t serialize() const;

protected:
	Key key_;
	char32_t utf32_;
};


}
