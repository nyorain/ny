#pragma once

#include <ny/include.hpp>
#include <ny/keyboardContext.hpp>
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

//TODO: unicode currently also writes for control keys like esc or ^C
//either filter this in this implementation or change the specification of
//the KeyboardContext and KeyEvent members.
//TODO: rethink public/protected
///Partial KeyboardContext implementation for backends that can be used with xkb.
class XkbKeyboardContext : public KeyboardContext
{
public:
	std::string utf8(Keycode) const override;

	//specific
	xkb_context& xkbContext() const { return *xkbContext_; }
	xkb_keymap& xkbKeymap() const { return *xkbKeymap_; }
	xkb_state& xkbState() const { return *xkbState_; }

	xkb_compose_table* xkbComposeTable() const { return xkbComposeTable_; }
	xkb_compose_state* xkbComposeState() const { return xkbComposeState_; }

	const std::bitset<256>& keyStates() const { return keyStates_; }

	///Fills the given KeyEvent depending on the KeyEvent::pressed member and the given
	///xkbcommon keycode. Does also trigger the onKey callback.
	///Returns false when the given keycode cancelled the current compose state, i.e.
	///if it does not generate any valid keysym.
	bool keyEvent(std::uint8_t keycode, KeyEvent& ev);

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
	[[deprecated("Should not be needed if keyEvent is always used correctly")]]
	bool feedComposeKey(unsigned int keysym);

	///Updates the modifier state from backend events.
	void updateState(const Vec3ui& mods, const Vec3ui& layouts);

protected:
	xkb_context* xkbContext_ = nullptr;
	xkb_keymap* xkbKeymap_ = nullptr;
	xkb_state* xkbState_ = nullptr;

	xkb_compose_table* xkbComposeTable_ = nullptr;
	xkb_compose_state* xkbComposeState_ = nullptr;

	std::bitset<256> keyStates_;
};

}
