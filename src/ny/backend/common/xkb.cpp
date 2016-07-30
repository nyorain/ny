#include <ny/backend/common/xkb.hpp>
#include <xkbcommon/xkbcommon.h>

#include <nytl/vec.hpp>
#include <stdexcept>

namespace ny
{

XKBKeyboardContext::XKBKeyboardContext()
{
	xkbContext_ = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if(!xkbContext_)
		throw std::runtime_error("ny::XKBKeyboardContext: failed to create xkb_context");
}

XKBKeyboardContext::~XKBKeyboardContext()
{
	if(xkbState_) xkb_state_unref(xkbState_);
	if(xkbKeymap_) xkb_keymap_unref(xkbKeymap_);
	if(xkbContext_) xkb_context_unref(xkbContext_);
}

void XKBKeyboardContext::createDefault()
{
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

void XKBKeyboardContext::updateKey(unsigned int code, bool pressed)
{
	xkb_state_update_key(xkbState_, code, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
	// auto keyid = static_cast<unsigned int>(xkbToKey(code));
	// keyStates_[keyid] = pressed;
}

void XKBKeyboardContext::updateState(const Vec3ui& mods, const Vec3ui& layouts)
{
	xkb_state_update_mask(xkbState_, mods.x, mods.y, mods.z, layouts.x, layouts.y, layouts.z);
}

std::string XKBKeyboardContext::unicode(Key key) const
{
	auto code = keyToXKB(key);
	char utf8[7];
	xkb_state_key_get_utf8(xkbState_, code, utf8, 7);
	return utf8;
}

}
