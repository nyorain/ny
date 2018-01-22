// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/wayland/include.hpp>
#include <ny/common/xkb.hpp>
#include <ny/mouseContext.hpp>

#include <nytl/vec.hpp>
#include <nytl/nonCopyable.hpp>

namespace ny {

/// Wayland MouseContext implementation
class WaylandMouseContext : public MouseContext, public nytl::NonMovable {
public:
	WaylandMouseContext(WaylandAppContext&, wl_seat&);
	~WaylandMouseContext();

	// MouseContext
	nytl::Vec2i position() const override { return position_; }
	bool pressed(MouseButton) const override;
	WindowContext* over() const override;

	// - wayland specific -
	WaylandAppContext& appContext() const { return appContext_;}

	wl_pointer* wlPointer() const { return wlPointer_; }
	unsigned int lastSerial() const { return lastSerial_; }

	/// This function does only actively change the cursor if it is currently over
	/// an owned surface or grabbed by this application.
	/// \param buf Defined the cursor content. If nullptr, the cursor is hidden
	/// \param hs The cursor hotspot. {0, 0} would be the top-left corner.
	/// \param size The cursor size. Only relevant for damaging the surface so should be
	/// any value equal to or greater than the actual buffer size.
	void cursorBuffer(wl_buffer* buf, nytl::Vec2i hs = {}, nytl::Vec2ui size = {2048, 2048}) const;

protected:
	void handleEnter(wl_pointer*, uint32_t serial, wl_surface*, wl_fixed_t x, wl_fixed_t y);
	void handleLeave(wl_pointer*, uint32_t serial, wl_surface*);
	void handleMotion(wl_pointer*, uint32_t time, wl_fixed_t x, wl_fixed_t y);
	void handleButton(wl_pointer*, uint32_t s, uint32_t t, uint32_t button, uint32_t pressed);
	void handleAxis(wl_pointer*, uint32_t time, uint32_t axis, wl_fixed_t value);
	void handleFrame(wl_pointer*);
	void handleAxisSource(wl_pointer*, uint32_t source);
	void handleAxisStop(wl_pointer*, uint32_t time, uint32_t axis);
	void handleAxisDiscrete(wl_pointer*, uint32_t axis, int32_t discrete);

protected:
	WaylandAppContext& appContext_;
	WaylandWindowContext* over_ {};
	nytl::Vec2i position_;
	wl_pointer* wlPointer_ {};
	std::bitset<8> buttonStates_;

	wl_surface* wlCursorSurface_ {};

	unsigned int lastSerial_ {};
	unsigned int cursorSerial_ {};
};


/// Wayland KeyboardContext implementation
class WaylandKeyboardContext : public XkbKeyboardContext {
public:
	WaylandKeyboardContext(WaylandAppContext&, wl_seat&);
	~WaylandKeyboardContext();

	bool pressed(Keycode) const override;
	WindowContext* focus() const override { return focus_; }

	// - wayland specific -
	bool withKeymap(); // whether the compositor sent a keymap

	wl_keyboard* wlKeyboard() const { return wlKeyboard_; }
	unsigned int lastSerial() const { return lastSerial_; }

protected:
	void handleKeymap(wl_keyboard*, uint32_t format, int32_t fd, uint32_t size);
	void handleEnter(wl_keyboard*, uint32_t serial, wl_surface* surface, wl_array* keys);
	void handleLeave(wl_keyboard*, uint32_t serial, wl_surface* surface);
	void handleKey(wl_keyboard*, uint32_t serial, uint32_t time, uint32_t key, uint32_t pressed);
	void handleModifiers(wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
	void handleRepeatInfo(wl_keyboard*, int32_t rate, int32_t delay);

	void repeatKey();

protected:
	WaylandAppContext& appContext_;
	WindowContext* focus_ {};
	wl_keyboard* wlKeyboard_ {};
	bool keymap_ {};
	unsigned int lastSerial_ {};

	struct {
		unsigned int rates {};
		unsigned int ratens {};
		unsigned int delays {};
		unsigned int delayns {};
	} repeat_;

	int timerfd_ {};
	nytl::UniqueConnection timerfdConn_ {};
	unsigned int repeatKey_ {};
};

} // namespace ny
