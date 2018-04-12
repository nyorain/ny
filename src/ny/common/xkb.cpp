// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/common/xkb.hpp>
#include <ny/key.hpp>

#include <nytl/vec.hpp>
#include <nytl/utf.hpp>
#include <nytl/flags.hpp>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <stdexcept>

namespace ny {

// utility
Keycode xkbToKey(xkb_keycode_t keycode) { return static_cast<Keycode>(keycode - 8); }
xkb_keycode_t keyToXkb(Keycode keycode) { return static_cast<unsigned int>(keycode) + 8; }

// Keyboardcontext
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
	static constexpr auto contextFailed = "ny::XkbKeyboardContext: failed to create xkb_context";
	static constexpr auto keymapFailed = "ny::XkbKeyboardContext: failed to create xkb_keymap";
	static constexpr auto stateFailed = "ny::XkbKeyboardContext: failed to create xkb_state";

	xkbContext_ = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if(!xkbContext_) throw std::runtime_error(contextFailed);

	struct xkb_rule_names rules {};

	rules.rules = getenv("XKB_DEFAULT_RULES");
	rules.model = getenv("XKB_DEFAULT_MODEL");
	rules.layout = getenv("XKB_DEFAULT_LAYOUT");
	rules.variant = getenv("XKB_DEFAULT_VARIANT");
	rules.options = getenv("XKB_DEFAULT_OPTIONS");

	xkbKeymap_ = xkb_map_new_from_names(xkbContext_, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if(!xkbKeymap_) throw std::runtime_error(keymapFailed);

	xkbState_ = xkb_state_new(xkbKeymap_);
	if(!xkbState_) throw std::runtime_error(stateFailed);
}

void XkbKeyboardContext::setupCompose()
{
	static constexpr auto localeFailed = "ny::XkbKeyboardContext::setupCompose: "
		"failed to retrieve locale using setlocale";
	static constexpr auto tableFailed = "ny::XkbKeyboardContext::setupCompose: "
		"failed to setup xkb compose table";
	static constexpr auto stateFailed = "ny::XkbKeyboardContext::setupCompose: "
		"failed to setup xkb compose state";

	// TODO: can be improved, fall back to other locales if needed
	// https://raw.githubusercontent.com/glfw/glfw/master/src/wl_init.c
	auto locale = setlocale(LC_CTYPE, nullptr);
	if(!locale) throw std::runtime_error(localeFailed);

	xkbComposeTable_ = xkb_compose_table_new_from_locale(xkbContext_, locale,
		XKB_COMPOSE_COMPILE_NO_FLAGS);
	if(!xkbComposeTable_) throw std::runtime_error(tableFailed);

	xkbComposeState_ = xkb_compose_state_new(xkbComposeTable_, XKB_COMPOSE_STATE_NO_FLAGS);
	if(!xkbComposeTable_) throw std::runtime_error(stateFailed);
}

void XkbKeyboardContext::updateKey(unsigned int code, bool pressed)
{
	xkb_state_update_key(xkbState_, code, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
}

void XkbKeyboardContext::updateState(nytl::Vec3ui mods, nytl::Vec3ui layouts)
{
	xkb_state_update_mask(xkbState_, mods[0], mods[1], mods[2], layouts[0], layouts[1], layouts[2]);
}

std::string XkbKeyboardContext::utf8(Keycode key) const
{
	auto code = keyToXkb(key);
	std::string ret;

	// create a dummy state to not interfer/copy any current state
	auto state = xkb_state_new(xkbKeymap_);

	auto needed = xkb_state_key_get_utf8(state, code, nullptr, 0) + 1;
	ret.resize(needed);
	xkb_state_key_get_utf8(state, code, &ret[0], ret.size());

	xkb_state_unref(state);
	return ret;
}

KeyboardModifiers XkbKeyboardContext::modifiers() const
{
	struct {
		const char* xkb;
		KeyboardModifier modifier;
	} mappings[] = {
		{"Shift", KeyboardModifier::shift},
		{"Lock", KeyboardModifier::capsLock},
		{"Control", KeyboardModifier::ctrl},
		{"Mod1", KeyboardModifier::alt},
		{"Mod2", KeyboardModifier::numLock},
		{"Mod4", KeyboardModifier::super}
	};

	KeyboardModifiers ret {};
	for(const auto& mapping : mappings)
		if(xkb_state_mod_name_is_active(&xkbState(), mapping.xkb, XKB_STATE_MODS_EFFECTIVE))
			ret |= mapping.modifier;

	return ret;
}

bool XkbKeyboardContext::handleKey(std::uint8_t keycode, bool pressed, Keycode& keycodeOut,
	std::string& utf8)
{
	keycodeOut = xkbToKey(keycode);
	auto keyuint = static_cast<unsigned int>(keycodeOut);
	if(keyuint > 255) throw std::logic_error("ny::XkbKeyboardContext::keyEvent: keycode > 255");

	keyStates_[keyuint] = pressed;

	auto keysym = xkb_state_key_get_one_sym(xkbState_, keycode);
	auto ret = true;
	auto composed = false;
	if(pressed) {
		xkb_compose_state_feed(xkbComposeState_, keysym);
		auto status = xkb_compose_state_get_status(xkbComposeState_);
		if(status == XKB_COMPOSE_CANCELLED) {
			xkb_compose_state_reset(xkbComposeState_);
			ret = false;
		} else if(status == XKB_COMPOSE_COMPOSED) {
			auto needed = xkb_compose_state_get_utf8(xkbComposeState_, nullptr, 0) + 1;
			utf8.resize(needed);
			xkb_compose_state_get_utf8(xkbComposeState_, &utf8[0], needed);
			xkb_compose_state_reset(xkbComposeState_);
			composed = true;
		}
	}

	if(!composed) {
		auto needed = xkb_state_key_get_utf8(xkbState_, keycode, nullptr, 0) + 1;
		utf8.resize(needed);
		xkb_state_key_get_utf8(xkbState_, keycode, &utf8[0], needed);
	}

	return ret;
}

} // namespace ny
