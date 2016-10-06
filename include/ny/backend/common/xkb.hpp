#pragma once

#include <ny/include.hpp>
#include <ny/backend/keyboardContext.hpp>
#include <bitset>

struct xkb_context;
struct xkb_keymap;
struct xkb_state;
using xkb_keysym_t = std::uint32_t;

namespace ny
{

Key xkbToKey(xkb_keysym_t code);
xkb_keysym_t keyToXkb(Key key);

///Partial KeyboardContext implementation for backends that can be used with xkb.
class XkbKeyboardContext : public KeyboardContext
{
public:
	std::string unicode(Key key) const override;

	//specific
	xkb_keymap& xkbKeymap() const { return *xkbKeymap_; }
	xkb_state& xkbState() const { return *xkbState_; }

protected:
	XkbKeyboardContext();
	~XkbKeyboardContext();

	void createDefault();

	void updateKey(unsigned int keybode, bool pressed);
	void updateState(const Vec3ui& mods, const Vec3ui& layouts);

protected:
	xkb_context* xkbContext_ = nullptr;
	xkb_keymap* xkbKeymap_ = nullptr;
	xkb_state* xkbState_ = nullptr;
};

}
