// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/config.hpp>

#ifndef NY_WithXkbcommon
	#error ny was built without xkbcommon. Do not include this header.
#endif

#include <ny/keyboardContext.hpp>
#include <nytl/nonCopyable.hpp>
#include <bitset>

struct xkb_context;
struct xkb_keymap;
struct xkb_state;
struct xkb_compose_table;
struct xkb_compose_state;

using xkb_keycode_t = std::uint32_t;

namespace ny {

Keycode xkbToKey(xkb_keycode_t code);
xkb_keycode_t keyToXkb(Keycode key);

// TODO: unicode currently also writes for control keys like esc or ^C
// either filter this in this implementation or change the specification of
// the KeyboardContext and KeyEvent members.
// TODO: rethink public/protected?

/// Partial KeyboardContext implementation for backends that can be used with xkb.
class XkbKeyboardContext : public KeyboardContext, public nytl::NonMovable {
public:
	std::string utf8(Keycode) const override;
	KeyboardModifiers modifiers() const override;

	//specific
	xkb_context& xkbContext() const { return *xkbContext_; }
	xkb_keymap& xkbKeymap() const { return *xkbKeymap_; }
	xkb_state& xkbState() const { return *xkbState_; }

	xkb_compose_table* xkbComposeTable() const { return xkbComposeTable_; }
	xkb_compose_state* xkbComposeState() const { return xkbComposeState_; }

	const std::bitset<256>& keyStates() const { return keyStates_; }

	/// Fills the given KeyEvent depending on the KeyEvent::pressed member and the given
	/// xkbcommon keycode. Does not trigger the onKey callback.
	/// Returns false when the given keycode cancelled the current compose state, i.e.
	/// if it does not generate any valid keysym.
	bool handleKey(std::uint8_t keycode, bool pressed, Keycode&, std::string& utf8);

protected:
	XkbKeyboardContext();
	~XkbKeyboardContext();

	/// Creates a default context with default keymap and state.
	void createDefault();

	/// Sets up default compose table and state
	void setupCompose();

	/// Updates the given key to the given bool value for the xkb state.
	/// Note that these calls must only be called when having the backends has no
	/// possibility to retrieve modifier information (for an updateState call) from
	/// the window system.
	void updateKey(unsigned int keycode, bool pressed);

	/// Updates the modifier state from backend events.
	void updateState(nytl::Vec3ui mods, nytl::Vec3ui layouts);

protected:
	xkb_context* xkbContext_ = nullptr;
	xkb_keymap* xkbKeymap_ = nullptr;
	xkb_state* xkbState_ = nullptr;

	xkb_compose_table* xkbComposeTable_ = nullptr;
	xkb_compose_state* xkbComposeState_ = nullptr;

	std::bitset<256> keyStates_;
};

} // namespace ny
