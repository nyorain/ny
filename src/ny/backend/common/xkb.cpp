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
Keycode xkbToKey(xkb_keycode_t keycode)
{
	return static_cast<Keycode>(keycode - 8);
}

xkb_keycode_t keyToXkb(Keycode keycode)
{
	return static_cast<unsigned int>(keycode) + 8;
}


//Keyboardcontext
XkbKeyboardContext::XkbKeyboardContext()
{
}

XkbKeyboardContext::~XkbKeyboardContext()
{
	if(xkbComposeTable_) xkb_compose_table_unref(xkbComposeTable_);
	if(xkbComposeState_) xkb_compose_state_unref(xkbComposeState_);

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
	if(!locale)
		throw std::runtime_error("XkbKC::setupCompose: failed to retrieve locale");

	xkbComposeTable_ = xkb_compose_table_new_from_locale(xkbContext_, locale, 
		XKB_COMPOSE_COMPILE_NO_FLAGS);
	if(!xkbComposeTable_)
		throw std::runtime_error("XkbKC::setupCompose: failed to setup compose table");

	xkbComposeState_ = xkb_compose_state_new(xkbComposeTable_, XKB_COMPOSE_STATE_NO_FLAGS);
	if(!xkbComposeTable_)
		throw std::runtime_error("XkbKC::setupCompose: failed to setup compose state");
}

void XkbKeyboardContext::updateKey(unsigned int code, bool pressed)
{
	xkb_state_update_key(xkbState_, code, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
}

void XkbKeyboardContext::updateState(const Vec3ui& mods, const Vec3ui& layouts)
{
	xkb_state_update_mask(xkbState_, mods.x, mods.y, mods.z, layouts.x, layouts.y, layouts.z);
}

bool XkbKeyboardContext::feedComposeKey(unsigned int keysym)
{
	if(xkb_compose_state_feed(xkbComposeState_, keysym) == XKB_COMPOSE_FEED_IGNORED) return true;
	return (xkb_compose_state_get_status(xkbComposeState_) != XKB_COMPOSE_CANCELLED);
}

std::string XkbKeyboardContext::utf8(Keycode key, bool currentState) const
{
	//TODO: composing here for currentState == true is not supported atm.
	//doing so would actually change the state. It would be required to manually save
	//the current pending compose sequence, then compose with the given key and then
	//reset the compose state to the saved sequence. Way too complex for a not that useful
	//feature (at least atm).
	auto code = keyToXkb(key);
	std::string ret;

	auto state = xkb_state_ref(xkbState_);
	if(!currentState) state = xkb_state_new(xkbKeymap_);

	auto needed = xkb_state_key_get_utf8(xkbState_, code, nullptr, 0) + 1;
	ret.resize(needed);
	
	xkb_state_unref(state);
	xkb_state_key_get_utf8(xkbState_, code, &ret[0], ret.size());
	return ret;
}

}
