// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/winapi/include.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/winapi/util.hpp>
#include <ny/mouseContext.hpp>
#include <ny/keyboardContext.hpp>

#include <map>
#include <string>

namespace ny {

/// Winapi MouseContext implementation.
class WinapiMouseContext : public MouseContext {
public:
	WinapiMouseContext(WinapiAppContext& context) : context_(context) {}
	WinapiMouseContext() = default;

	nytl::Vec2i position() const override;
	bool pressed(MouseButton button) const override;
	WinapiWindowContext* over() const override { return over_; }

	// - winapi specific -
	/// Handles the given event if it is mouse related. Returns false otherwise.
	/// If it could be handled sets the result value.
	bool processEvent(const WinapiEventData&, LRESULT& result);
	void destroyed(const WinapiWindowContext&);

protected:
	WinapiAppContext& context_;
	WinapiWindowContext* over_ {};
	nytl::Vec2i position_;
};

/// Winapi KeyboardContext implementation.
class WinapiKeyboardContext : public KeyboardContext {
public:
	WinapiKeyboardContext(WinapiAppContext& context);
	WinapiKeyboardContext() = default;

	bool pressed(Keycode) const override;
	std::string utf8(Keycode) const override;
	WinapiWindowContext* focus() const override { return focus_; }
	KeyboardModifiers modifiers() const override;

	// - winapi specific -
	/// Handles the given event if it is keyboard related. Returns false otherwise.
	/// If it could be handled sets the result value.
	bool processEvent(const WinapiEventData&, LRESULT& result);

	/// Returns whether the given virtual keycode is pressed using GetAsyncKeyState.
	bool pressed(unsigned int vkcode) const;
	void destroyed(const WinapiWindowContext&);

protected:
	WinapiAppContext& context_;
	WinapiWindowContext* focus_ {};
	std::map<Keycode, std::string> keycodeUnicodeMap_;
};

} // namespace ny
