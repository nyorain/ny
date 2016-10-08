#include <ny/backend/common/xkb.hpp>

#include <nytl/vec.hpp>
#include <nytl/utf.hpp>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <stdexcept>

namespace ny
{

//utility
Key xkbToKey(xkb_keysym_t keysym)
{

	//0 - 9 keypad
	if(keysym >= XKB_KEY_KP_0 && keysym <= XKB_KEY_KP_9) 
		return static_cast<Key>(keysym - XKB_KEY_KP_0 + static_cast<unsigned int>(Key::n0));

	//f1 - f24
	if(keysym >= XKB_KEY_F1 && keysym <= XKB_KEY_F24) 
		return static_cast<Key>(keysym - XKB_KEY_F1 + static_cast<unsigned int>(Key::f1));

	//0 - 9
	if(keysym >= XKB_KEY_0 && keysym <= XKB_KEY_9) 
		return static_cast<Key>(keysym - XKB_KEY_0 + static_cast<unsigned int>(Key::n0));

	//A - Z
	if(keysym >= XKB_KEY_A && keysym <= XKB_KEY_Z) 
		return static_cast<Key>(keysym - XKB_KEY_A + static_cast<unsigned int>(Key::a));

	//a - z
	if(keysym >= XKB_KEY_a && keysym <= XKB_KEY_z) 
		return static_cast<Key>(keysym - XKB_KEY_a + static_cast<unsigned int>(Key::a));

	switch(keysym)
	{
		case XKB_KEY_BackSpace: return Key::backspace;
		case XKB_KEY_Tab: return Key::tab;
		case XKB_KEY_Return: return Key::enter;
		case XKB_KEY_Pause: return Key::pause;
		case XKB_KEY_Escape: return Key::escape;
		case XKB_KEY_Delete: return Key::del;

		case XKB_KEY_Home: return Key::home;
		case XKB_KEY_Left: return Key::left;
		case XKB_KEY_Right: return Key::right;
		case XKB_KEY_Down: return Key::down;
		case XKB_KEY_Page_Up: return Key::pageUp;
		case XKB_KEY_Page_Down: return Key::pageDown;

		case XKB_KEY_KP_Space: return Key::space;
		case XKB_KEY_KP_Tab: return Key::tab;
		case XKB_KEY_KP_Enter: return Key::enter;
		case XKB_KEY_KP_F1: return Key::f1;
		case XKB_KEY_KP_F2: return Key::f2;
		case XKB_KEY_KP_F3: return Key::f3;
		case XKB_KEY_KP_F4: return Key::f4;
		case XKB_KEY_KP_Home: return Key::home;
		case XKB_KEY_KP_Left: return Key::left;
		case XKB_KEY_KP_Right: return Key::right;
		case XKB_KEY_KP_Down: return Key::down;
		case XKB_KEY_KP_Page_Up: return Key::pageUp;
		case XKB_KEY_KP_Page_Down: return Key::pageDown;
		case XKB_KEY_KP_Delete: return Key::del;

		case XKB_KEY_Shift_L: return Key::leftshift;
		case XKB_KEY_Shift_R: return Key::rightshift;
		case XKB_KEY_Control_L: return Key::leftctrl;
		case XKB_KEY_Control_R: return Key::rightctrl;
		case XKB_KEY_Caps_Lock: return Key::capsLock;
		case XKB_KEY_Alt_L: return Key::leftalt;
		case XKB_KEY_Alt_R: return Key::rightalt;
		case XKB_KEY_Super_L: return Key::leftsuper;
		case XKB_KEY_Super_R: return Key::rightsuper;

		case XKB_KEY_space: return Key::space;

		default: return Key::none;
	}
}

xkb_keysym_t keyToXkb(Key key)
{
	//TODO
	switch(key)
	{
		default: return 0;
	}
}


//Keyboardcontext
XkbKeyboardContext::XkbKeyboardContext()
{
}

XkbKeyboardContext::~XkbKeyboardContext()
{
	if(xkbState_) xkb_state_unref(xkbState_);
	if(xkbKeymap_) xkb_keymap_unref(xkbKeymap_);
	if(xkbContext_) xkb_context_unref(xkbContext_);
}

void XkbKeyboardContext::createDefault()
{
	xkbContext_ = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if(!xkbContext_)
		throw std::runtime_error("ny::XKBKeyboardContext: failed to create xkb_context");

	struct xkb_rule_names rules {};

	rules.rules = getenv("XKB_DEFAULT_RULES");
	rules.model = getenv("XKB_DEFAULT_MODEL");
    rules.layout = getenv("XKB_DEFAULT_LAYOUT");
	rules.variant = getenv("XKB_DEFAULT_VARIANT");
	rules.options = getenv("XKB_DEFAULT_OPTIONS");

	xkbKeymap_ = xkb_map_new_from_names(xkbContext_, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if(!xkbKeymap_)
		throw std::runtime_error("ny::XKBKeyboardContext: failed to create xkb_keymap");

	xkbState_ = xkb_state_new(xkbKeymap_);
	if(!xkbState_)
		throw std::runtime_error("ny::XKBKeyboardContext: failed to create xkb_state");
}

void XkbKeyboardContext::setupCompose()
{
	auto locale = setlocale(LC_CTYPE, nullptr);
	xkbComposeTable_ = xkb_compose_table_new_from_locale(xkbContext_, locale, XKB_COMPOSE_COMPILE_NO_FLAGS);
	xkbComposeState_ = xkb_compose_state_new(xkbComposeTable_, XKB_COMPOSE_STATE_NO_FLAGS);
}

void XkbKeyboardContext::updateKey(unsigned int code, bool pressed)
{
	xkb_state_update_key(xkbState_, code, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
	// auto keyid = static_cast<unsigned int>(xkbToKey(code));
	// keyStates_[keyid] = pressed;
}

void XkbKeyboardContext::updateState(const Vec3ui& mods, const Vec3ui& layouts)
{
	xkb_state_update_mask(xkbState_, mods.x, mods.y, mods.z, layouts.x, layouts.y, layouts.z);
}

std::string XkbKeyboardContext::unicode(Key key) const
{
	auto code = keyToXkb(key);
	char utf8[7];
	xkb_state_key_get_utf8(xkbState_, code, utf8, 7);
	
	return utf8;
}


// Keycode keycodeFromUtf32(char32_t utf32) const
// {
// }
// 
// Keycode keycodeFromUtf8(const std::array<char, 4>& utf8) const
// {
// }
// 
// char32_t keycodeToUtf32(Keycode, bool currentState = false) const
// {
// 	auto state = xkb_state_ref(xkbState_);
// 	if(!currentState) state = xkb_state_new(xkbKeymap_);
// 
// 	auto ret = xkb_state_key_get_utf8(state, code);
// 	state = xkb_state_unref(xkbKeymap_);
// 
// 	return ret;
// }
// 
// std::array<char, 4> keycodeToUtf8(Keycode keycode, bool currentState = false) const
// {
// 	auto state = xkb_state_ref(xkbState_);
// 	if(!currentState) state = xkb_state_new(xkbKeymap_);
// 
// 	char utf8[5];
// 	xkb_state_key_get_utf8(state, code, utf8, 5);
// 	state = xkb_state_unref(xkbKeymap_);
// 
// 	return utf8;
// }

}
