#pragma once

#include <ny/include.hpp>
#include <ny/backend/keyboardContext.hpp>
#include <bitset>

struct xkb_context;
struct xkb_keymap;
struct xkb_state;
struct xkb_compose_table;
struct xkb_compose_state;

using xkb_keycode_t = std::uint32_t;

namespace ny
{

Keycode xkbToKey(xkb_keycode_t code);
xkb_keycode_t keyToXkb(Keycode key);

///Partial KeyboardContext implementation for backends that can be used with xkb.
class XkbKeyboardContext : public KeyboardContext
{
public:
	std::string utf8(Keycode, bool currentState = false) const override;

	//specific
	xkb_context& xkbContext() const { return *xkbContext_; }
	xkb_keymap& xkbKeymap() const { return *xkbKeymap_; }
	xkb_state& xkbState() const { return *xkbState_; }

protected:
	XkbKeyboardContext();
	~XkbKeyboardContext();

	///Creates a default context with default keymap and state.
	void createDefault();

	///Sets up default compose table and state
	void setupCompose();

	///Updates the given key to the given bool value for the xkb state.
	///Note that these calls must only be called when having the backends has no
	///possibility to retrieve modifier information (for an updateState call) from
	///the window system.
	void updateKey(unsigned int keycode, bool pressed);

	///Feeds the keysym to the compose state machine.
	///Returns false if the keysym cancelled the current sequence.
	bool feedComposeKey(unsigned int keysym);

	///Updates the modifier state from backend events.
	void updateState(const Vec3ui& mods, const Vec3ui& layouts);

protected:
	xkb_context* xkbContext_ = nullptr;
	xkb_keymap* xkbKeymap_ = nullptr;
	xkb_state* xkbState_ = nullptr;

	xkb_compose_table* xkbComposeTable_ = nullptr;
	xkb_compose_state* xkbComposeState_ = nullptr;
};

}
