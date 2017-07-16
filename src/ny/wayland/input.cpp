// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/input.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/util.hpp>
#include <ny/common/unix.hpp>
#include <ny/windowContext.hpp>
#include <ny/log.hpp>

#include <nytl/span.hpp>
#include <nytl/scope.hpp>

#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>
#include <unistd.h>
#include <sys/mman.h>

namespace ny {

// mouse
WaylandMouseContext::WaylandMouseContext(WaylandAppContext& ac, wl_seat& seat)
	: appContext_(ac)
{
	using WMC = WaylandMouseContext;
	constexpr static wl_pointer_listener listener = {
		memberCallback<decltype(&WMC::handleEnter), &WMC::handleEnter>,
		memberCallback<decltype(&WMC::handleLeave), &WMC::handleLeave>,
		memberCallback<decltype(&WMC::handleMotion), &WMC::handleMotion>,
		memberCallback<decltype(&WMC::handleButton), &WMC::handleButton>,
		memberCallback<decltype(&WMC::handleAxis), &WMC::handleAxis>,
		memberCallback<decltype(&WMC::handleFrame), &WMC::handleFrame>,
		memberCallback<decltype(&WMC::handleAxisSource), &WMC::handleAxisSource>,
		memberCallback<decltype(&WMC::handleAxisStop), &WMC::handleAxisStop>,
		memberCallback<decltype(&WMC::handleAxisDiscrete), &WMC::handleAxisDiscrete>,
	};

	wlPointer_ = wl_seat_get_pointer(&seat);
	wl_pointer_add_listener(wlPointer_, &listener, this);
	wlCursorSurface_ = wl_compositor_create_surface(&ac.wlCompositor());
}

WaylandMouseContext::~WaylandMouseContext()
{
	if(wlPointer_) {
		if(wl_pointer_get_version(wlPointer_) >= 3) wl_pointer_release(wlPointer_);
		else wl_pointer_destroy(wlPointer_);
	}
	if(wlCursorSurface_) wl_surface_destroy(wlCursorSurface_);
}

bool WaylandMouseContext::pressed(MouseButton button) const
{
	return buttonStates_[static_cast<unsigned int>(button)];
}

WindowContext* WaylandMouseContext::over() const
{
	return over_;
}

void WaylandMouseContext::handleMotion(wl_pointer*, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	nytl::unused(time);

	auto oldPos = position_;
	position_ = {wl_fixed_to_int(x), wl_fixed_to_int(y)};
	auto delta = position_ - oldPos;
	onMove(*this, position_, delta);

	if(over_) {
		MouseMoveEvent mme;
		mme.position = position_;
		mme.delta = delta;
		over_->listener().mouseMove(mme);
	}
}
void WaylandMouseContext::handleEnter(wl_pointer*, uint32_t serial, wl_surface* surface,
	wl_fixed_t x, wl_fixed_t y)
{
	auto pos = nytl::Vec2i {wl_fixed_to_int(x), wl_fixed_to_int(y)};

	lastSerial_ = serial;
	cursorSerial_ = serial;

	auto wc = appContext_.windowContext(*surface);
	WaylandEventData eventData(serial);

	if(wc != over_) {
		onFocus(*this, over_, wc);

		MouseCrossEvent mce;
		mce.eventData = &eventData;
		mce.entered = false;
		mce.position = position_;
		wc->listener().mouseCross(mce);

		if(over_) over_->listener().mouseCross(mce);

		if(wc) {
			mce.entered = true;
			mce.position = pos;
			wc->listener().mouseCross(mce);
			cursorBuffer(wc->wlCursorBuffer(), wc->cursorHotspot(), wc->cursorSize());
		}

		over_ = wc;
	}

	position_ = pos;
}
void WaylandMouseContext::handleLeave(wl_pointer*, uint32_t serial, wl_surface* surface)
{
	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	auto wc = appContext_.windowContext(*surface);
	if(wc != over_) ny_warn("::WlMouseContext::handleLeave"_src, "'over_' inconsistency");

	if(over_) onFocus(*this, over_, nullptr);
	if(wc) {
		MouseCrossEvent mce;
		mce.eventData = &eventData;
		mce.entered = false;
		mce.position = position_;
		wc->listener().mouseCross(mce);
	}

	over_ = nullptr;
}
void WaylandMouseContext::handleButton(wl_pointer*, uint32_t serial, uint32_t time, uint32_t button,
	uint32_t pressed)
{
	nytl::unused(time);

	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	auto nybutton = linuxToButton(button);
	onButton(*this, nybutton, pressed);

	if(over_) {
		MouseButtonEvent mbe;
		mbe.eventData = &eventData;
		mbe.position = position_;
		mbe.pressed = pressed;
		mbe.button = nybutton;
		over_->listener().mouseButton(mbe);
	}
}

void WaylandMouseContext::handleAxis(wl_pointer*, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	nytl::unused(time, axis);
	auto nvalue = wl_fixed_to_int(value);
	onWheel(*this, nvalue);

	if(over_) {
		MouseWheelEvent mwe;
		mwe.value = nvalue;
		mwe.position = position_;
		over_->listener().mouseWheel(mwe);
	}
}

// Those events could be handled if more complex axis events are supported.
// Handling the fame event will only become useful when the 3 callbacks below are handled
void WaylandMouseContext::handleFrame(wl_pointer*)
{
}

void WaylandMouseContext::handleAxisSource(wl_pointer*, uint32_t source)
{
	nytl::unused(source);
}

void WaylandMouseContext::handleAxisStop(wl_pointer*, uint32_t time, uint32_t axis)
{
	nytl::unused(time, axis);
}

void WaylandMouseContext::handleAxisDiscrete(wl_pointer*, uint32_t axis, int32_t discrete)
{
	nytl::unused(axis, discrete);
}

void WaylandMouseContext::cursorBuffer(wl_buffer* buf, nytl::Vec2i hs, nytl::Vec2ui size) const
{
	wl_pointer_set_cursor(wlPointer_, cursorSerial_, wlCursorSurface_, hs[0], hs[1]);

	wl_surface_attach(wlCursorSurface_, buf, 0, 0);
	wl_surface_damage(wlCursorSurface_, 0, 0, size[0], size[1]);
	wl_surface_commit(wlCursorSurface_);
}

//keyboard
WaylandKeyboardContext::WaylandKeyboardContext(WaylandAppContext& ac, wl_seat& seat)
	: appContext_(ac)
{
	using WKC = WaylandKeyboardContext;
	constexpr static wl_keyboard_listener listener = {
		memberCallback<decltype(&WKC::handleKeymap), &WKC::handleKeymap>,
		memberCallback<decltype(&WKC::handleEnter), &WKC::handleEnter>,
		memberCallback<decltype(&WKC::handleLeave), &WKC::handleLeave>,
		memberCallback<decltype(&WKC::handleKey), &WKC::handleKey>,
		memberCallback<decltype(&WKC::handleModifiers), &WKC::handleModifiers>,
		memberCallback<decltype(&WKC::handleRepeatInfo), &WKC::handleRepeatInfo>,
	};

	wlKeyboard_ = wl_seat_get_keyboard(&seat);
	wl_keyboard_add_listener(wlKeyboard_, &listener, this);

	XkbKeyboardContext::createDefault();
	XkbKeyboardContext::setupCompose();
}

WaylandKeyboardContext::~WaylandKeyboardContext()
{
	if(wlKeyboard_) {
		if(wl_keyboard_get_version(wlKeyboard_) >= 3) wl_keyboard_release(wlKeyboard_);
		else wl_keyboard_destroy(wlKeyboard_);
	}
}

bool WaylandKeyboardContext::pressed(Keycode key) const
{
	auto uint = static_cast<unsigned int>(key);
	if(uint > 255) return false;
	return keyStates_[uint];
}

bool WaylandKeyboardContext::withKeymap()
{
	return keymap_;
}
void WaylandKeyboardContext::handleKeymap(wl_keyboard*, uint32_t format, int32_t fd, uint32_t size)
{
	dlg_source("wlkc"_module, "handleKeymap"_scope);

	// always close the give fd
	auto fdGuard = nytl::ScopeGuard([=]{ if(fd) close(fd); });

	if(format == WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP) return;
	if(format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		ny_warn("invalid keymap format");
		return;
	}

	auto buf = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
	if(buf == MAP_FAILED) {
		ny_warn("Wl: cannot mmap keymap");
		return;
	}

	//always unmap the buffer
	auto mapGuard = nytl::ScopeGuard([=]{ munmap(buf, size); });

	keymap_ = true;

	if(xkbState_) xkb_state_unref(xkbState_);
	if(xkbKeymap_) xkb_keymap_unref(xkbKeymap_);

	auto data = static_cast<char*>(buf);
	xkbKeymap_ = xkb_keymap_new_from_buffer(xkbContext_, data, size - 1,
		XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

	if(!xkbKeymap_) {
		ny_warn("failed to compile the xkb keymap from compositor.");
		return;
	}

	xkbState_ = xkb_state_new(xkbKeymap_);

	if(!xkbState_) {
		ny_warn("failed to create the xkbState from mapped keymap buffer");
		return;
	}
}
void WaylandKeyboardContext::handleEnter(wl_keyboard*, uint32_t serial, wl_surface* surface,
	wl_array* keys)
{
	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	keyStates_.reset();
	for(auto i = 0u; i < keys->size / sizeof(uint32_t); ++i) {
		auto keyid = (static_cast<uint32_t*>(keys->data))[i];
		keyStates_[keyid] = true;
	}

	auto* wc = appContext_.windowContext(*surface);
	if(wc != focus_) {
		onFocus(*this, focus_, wc);

		FocusEvent fe;
		fe.gained = false;
		fe.eventData = &eventData;

		if(focus_) focus_->listener().focus(fe);
		if(wc) {
			fe.gained = true;
			wc->listener().focus(fe);
		}

		focus_ = wc;
	}
}

void WaylandKeyboardContext::handleLeave(wl_keyboard*, uint32_t serial, wl_surface* surface)
{
	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	auto* wc = appContext_.windowContext(*surface);
	if(wc != focus_) ny_warn("::WlKeyboardContext::handleLeave"_src, "'focus_' inconsistency");

	if(wc) {
		onFocus(*this, wc, nullptr);
		FocusEvent fe;
		fe.gained = false;
		fe.eventData = &eventData;
		wc->listener().focus(fe);
	}

	keyStates_.reset();
	focus_ = nullptr;
}
void WaylandKeyboardContext::handleKey(wl_keyboard*, uint32_t serial, uint32_t time,
	uint32_t key, uint32_t pressed)
{
	nytl::unused(time);

	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	Keycode keycode;
	std::string utf8;

	XkbKeyboardContext::handleKey(key + 8, pressed, keycode, utf8);
	if(focus_) {
		KeyEvent ke;
		ke.eventData = &eventData;
		ke.keycode = keycode;
		ke.utf8 = utf8;
		ke.pressed = pressed;
		focus_->listener().key(ke);
	}

	onKey(*this, keycode, utf8, pressed);
}

void WaylandKeyboardContext::handleModifiers(wl_keyboard*, uint32_t serial, uint32_t mdepressed,
	uint32_t mlatched, uint32_t mlocked, uint32_t group)
{
	lastSerial_ = serial;
	XkbKeyboardContext::updateState({mdepressed, mlatched, mlocked}, {group, group, group});
}

void WaylandKeyboardContext::handleRepeatInfo(wl_keyboard*, int32_t rate, int32_t delay)
{
	nytl::unused(rate, delay);

	repeatRate_ = rate;
	repeatDelay_ = delay;
}

} // namespace ny
