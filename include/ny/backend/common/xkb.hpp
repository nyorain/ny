#pragma once

#include <ny/include.hpp>
#include <ny/backend/keyboardContext.hpp>
#include <bitset>

struct xkb_context;
struct xkb_keymap;
struct xkb_state;
struct xkb_compose_table;
struct xkb_compose_state;

using xkb_keysym_t = std::uint32_t;

namespace ny
{

Key xkbToKey(xkb_keysym_t code);
xkb_keysym_t keyToXkb(Key key);

///Partial KeyboardContext implementation for backends that can be used with xkb.
class XkbKeyboardContext : public KeyboardContext
{
public:
	// virtual Keycode keycodeFromUtf32(char32_t utf32) const;
	// virtual Keycode keycodeFromUtf8(const std::array<char, 4>& utf8) const;
	// virtual char32_t keycodeToUtf32(Keycode, bool currentState = false) const;
	// virtual std::array<char, 4> keycodeToUtf8(Keycode, bool currentState = false) const;

	std::string unicode(Key key) const override;

	//specific
	xkb_context& xkbContext() const { return *xkbContext_; }
	xkb_keymap& xkbKeymap() const { return *xkbKeymap_; }
	xkb_state& xkbState() const { return *xkbState_; }

protected:
	XkbKeyboardContext();
	~XkbKeyboardContext();

	///Creates a default context with default keymap and state.
	void createDefault();

	///Sets up compose table and state
	void setupCompose();

	///Updates the given key to the given bool value for the xkb state.
	///Note that these calls must only be called when having the backends has no
	///possibility to retrieve modifier information (for an updateState call) from
	///the window system.
	void updateKey(unsigned int keybode, bool pressed);

	///Updates the modifier state.
	void updateState(const Vec3ui& mods, const Vec3ui& layouts);

protected:
	xkb_context* xkbContext_ = nullptr;
	xkb_keymap* xkbKeymap_ = nullptr;
	xkb_state* xkbState_ = nullptr;

	xkb_compose_table* xkbComposeTable_ = nullptr;
	xkb_compose_state* xkbComposeState_ = nullptr;
};

}
