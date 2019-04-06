// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/wayland/input.hpp>
#include <ny/wayland/appContext.hpp>
#include <ny/wayland/windowContext.hpp>
#include <ny/wayland/util.hpp>
#include <ny/common/unix.hpp>
#include <ny/windowContext.hpp>
#include <dlg/dlg.hpp>

#include <nytl/span.hpp>
#include <nytl/scope.hpp>
#include <nytl/tmpUtil.hpp> // nytl::unused

#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>
#include <unistd.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/timerfd.h>

namespace ny {

// mouse
WaylandMouseContext::WaylandMouseContext(WaylandAppContext& ac, wl_seat& seat)
		: appContext_(ac) {
	using WMC = WaylandMouseContext;
	constexpr static wl_pointer_listener listener = {
		memberCallback<&WMC::handleEnter>,
		memberCallback<&WMC::handleLeave>,
		memberCallback<&WMC::handleMotion>,
		memberCallback<&WMC::handleButton>,
		memberCallback<&WMC::handleAxis>,
		memberCallback<&WMC::handleFrame>,
		memberCallback<&WMC::handleAxisSource>,
		memberCallback<&WMC::handleAxisStop>,
		memberCallback<&WMC::handleAxisDiscrete>,
	};

	wlPointer_ = wl_seat_get_pointer(&seat);
	wl_pointer_add_listener(wlPointer_, &listener, this);
	wlCursorSurface_ = wl_compositor_create_surface(&ac.wlCompositor());
}

WaylandMouseContext::~WaylandMouseContext() {
	if(wlPointer_) {
		if(wl_pointer_get_version(wlPointer_) >= 3) {
			wl_pointer_release(wlPointer_);
		} else {
			wl_pointer_destroy(wlPointer_);
		}
	}

	if(wlCursorSurface_) {
		wl_surface_destroy(wlCursorSurface_);
	}
}

bool WaylandMouseContext::pressed(MouseButton button) const {
	return buttonStates_[static_cast<unsigned int>(button)];
}

WindowContext* WaylandMouseContext::over() const {
	return over_;
}

void WaylandMouseContext::handleMotion(wl_pointer*, uint32_t time,
		wl_fixed_t x, wl_fixed_t y) {
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

void WaylandMouseContext::handleEnter(wl_pointer*, uint32_t serial,
		wl_surface* surface, wl_fixed_t x, wl_fixed_t y) {
	auto pos = nytl::Vec2i {wl_fixed_to_int(x), wl_fixed_to_int(y)};

	lastSerial_ = serial;
	cursorSerial_ = serial;

	auto wc = appContext_.windowContext(*surface);
	WaylandEventData eventData(serial);

	if(wc != over_) {
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
void WaylandMouseContext::handleLeave(wl_pointer*, uint32_t serial,
		wl_surface* surface) {
	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	auto wc = appContext_.windowContext(*surface);
	if(wc != over_) {
		dlg_warn("'over_' inconsistency");
	}
	if(wc) {
		MouseCrossEvent mce;
		mce.eventData = &eventData;
		mce.entered = false;
		mce.position = position_;
		wc->listener().mouseCross(mce);
	}

	over_ = nullptr;
}
void WaylandMouseContext::handleButton(wl_pointer*, uint32_t serial,
		uint32_t time, uint32_t button, uint32_t pressed) {
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

void WaylandMouseContext::handleAxis(wl_pointer*, uint32_t time, uint32_t axis,
		wl_fixed_t value) {
	nytl::unused(time);

	float nvalue = wl_fixed_to_double(value) / 10.f;
	nytl::Vec2f scroll {};
	if(axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
		scroll = {nvalue, 0.f};
	} else if(axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
		scroll = {0.f, nvalue};
	} else {
		dlg_info("Wayland: unsupported axis {}", axis);
	}

	onWheel(*this, scroll);

	if(over_) {
		MouseWheelEvent mwe;
		mwe.value = scroll;
		mwe.position = position_;
		over_->listener().mouseWheel(mwe);
	}
}

// Those events could be handled if more complex axis events are supported.
// Handling the frame event will only become useful when the 3 callbacks
// below are handled
void WaylandMouseContext::handleFrame(wl_pointer*) {
}

void WaylandMouseContext::handleAxisSource(wl_pointer*, uint32_t source) {
	nytl::unused(source);
}

void WaylandMouseContext::handleAxisStop(wl_pointer*, uint32_t time, uint32_t axis) {
	nytl::unused(time, axis);
}

void WaylandMouseContext::handleAxisDiscrete(wl_pointer*, uint32_t axis,
		int32_t discrete) {
	nytl::unused(axis, discrete);
}

void WaylandMouseContext::cursorBuffer(wl_buffer* buf, nytl::Vec2i hs,
		nytl::Vec2ui size) const {
	wl_pointer_set_cursor(wlPointer_, cursorSerial_, wlCursorSurface_, hs[0], hs[1]);

	wl_surface_attach(wlCursorSurface_, buf, 0, 0);
	wl_surface_damage(wlCursorSurface_, 0, 0, size[0], size[1]);
	wl_surface_commit(wlCursorSurface_);
}

void WaylandMouseContext::destroyed(const WaylandWindowContext& wc) {
	if(over_ == &wc) {
		over_ = nullptr;
	}
}

//keyboard
WaylandKeyboardContext::WaylandKeyboardContext(WaylandAppContext& ac,
		wl_seat& seat) : appContext_(ac) {
	using WKC = WaylandKeyboardContext;
	constexpr static wl_keyboard_listener listener = {
		memberCallback<&WKC::handleKeymap>,
		memberCallback<&WKC::handleEnter>,
		memberCallback<&WKC::handleLeave>,
		memberCallback<&WKC::handleKey>,
		memberCallback<&WKC::handleModifiers>,
		memberCallback<&WKC::handleRepeatInfo>,
	};

	wlKeyboard_ = wl_seat_get_keyboard(&seat);
	wl_keyboard_add_listener(wlKeyboard_, &listener, this);

	XkbKeyboardContext::createDefault();
	XkbKeyboardContext::setupCompose();

	// TODO: error checking
	timerfd_ = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
	timerfdConn_ = ac.fdCallback(timerfd_, POLLIN, [=](int, unsigned int){
		this->repeatKey();
		return true;
	});
}

WaylandKeyboardContext::~WaylandKeyboardContext() {
	if(timerfd_) {
		close(timerfd_);
	}

	if(wlKeyboard_) {
		if(wl_keyboard_get_version(wlKeyboard_) >= 3) {
			wl_keyboard_release(wlKeyboard_);
		} else {
			wl_keyboard_destroy(wlKeyboard_);
		}
	}
}

bool WaylandKeyboardContext::pressed(Keycode key) const {
	auto uint = static_cast<unsigned int>(key);
	if(uint > 255) {
		return false;
	}

	return keyStates_[uint];
}

bool WaylandKeyboardContext::withKeymap() {
	return keymap_;
}

void WaylandKeyboardContext::handleKeymap(wl_keyboard*, uint32_t format,
		int32_t fd, uint32_t size) {
	// always close the give fd
	auto fdGuard = nytl::ScopeGuard([=]{ if(fd) close(fd); });

	if(format == WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP) {
		return;
	}
	if(format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		dlg_warn("invalid keymap format");
		return;
	}

	auto buf = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
	if(buf == MAP_FAILED) {
		dlg_warn("Wl: cannot mmap keymap");
		return;
	}

	//always unmap the buffer
	auto mapGuard = nytl::ScopeGuard([=]{ munmap(buf, size); });

	keymap_ = true;

	if(xkbState_) {
		xkb_state_unref(xkbState_);
	}
	if(xkbKeymap_) {
		xkb_keymap_unref(xkbKeymap_);
	}

	auto data = static_cast<char*>(buf);
	xkbKeymap_ = xkb_keymap_new_from_buffer(xkbContext_, data, size - 1,
		XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

	if(!xkbKeymap_) {
		dlg_warn("failed to compile the xkb keymap from compositor.");
		return;
	}

	xkbState_ = xkb_state_new(xkbKeymap_);

	if(!xkbState_) {
		dlg_warn("failed to create the xkbState from mapped keymap buffer");
		return;
	}
}
void WaylandKeyboardContext::handleEnter(wl_keyboard*, uint32_t serial,
		wl_surface* surface, wl_array* keys) {
	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	keyStates_.reset();
	for(auto i = 0u; i < keys->size / sizeof(uint32_t); ++i) {
		auto keyid = (static_cast<uint32_t*>(keys->data))[i];
		keyStates_[keyid] = true;
	}

	auto* wc = appContext_.windowContext(*surface);
	if(wc != focus_) {
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

void WaylandKeyboardContext::handleLeave(wl_keyboard*, uint32_t serial,
		wl_surface* surface) {
	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	auto* wc = appContext_.windowContext(*surface);
	if(wc != focus_) {
		dlg_warn("'focus_' inconsistency");
	}

	if(wc) {
		FocusEvent fe;
		fe.gained = false;
		fe.eventData = &eventData;
		wc->listener().focus(fe);
	}

	keyStates_.reset();
	focus_ = nullptr;

	// reset repeat
	struct itimerspec its;
	repeatKey_ = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	timerfd_settime(timerfd_, 0, &its, nullptr);
}
void WaylandKeyboardContext::handleKey(wl_keyboard*, uint32_t serial,
		uint32_t time, uint32_t key, uint32_t pressed) {
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
		ke.repeat = false;
		focus_->listener().key(ke);
	}

	// repeat the key
	struct itimerspec its;
	if(pressed && xkb_keymap_key_repeats(xkbKeymap_, key + 8)) {
		repeatKey_ = key;
		its.it_interval.tv_sec = repeat_.rates;
		its.it_interval.tv_nsec = repeat_.ratens;
		its.it_value.tv_sec = repeat_.delays;
		its.it_value.tv_nsec = repeat_.delayns;
		timerfd_settime(timerfd_, 0, &its, nullptr);
	} else if(!pressed && key == repeatKey_) {
		repeatKey_ = 0;
		its.it_interval.tv_sec = 0;
		its.it_interval.tv_nsec = 0;
		its.it_value.tv_sec = 0;
		its.it_value.tv_nsec = 0;
		timerfd_settime(timerfd_, 0, &its, nullptr);
	}
}

void WaylandKeyboardContext::handleModifiers(wl_keyboard*, uint32_t serial,
		uint32_t mdepressed, uint32_t mlatched, uint32_t mlocked,
		uint32_t group) {
	lastSerial_ = serial;
	XkbKeyboardContext::updateState({mdepressed, mlatched, mlocked}, {group, group, group});
}

void WaylandKeyboardContext::handleRepeatInfo(wl_keyboard*, int32_t rate,
		int32_t delay) {
	repeat_ = {};

	if(rate == 0) { // repeating disabled
		return;
	}

	auto& r = repeat_;
	if (rate == 1) {
		r.rates = 1;
	} else {
		r.ratens = 1000000000 / rate;
	}

	r.delays = delay / 1000;
	delay -= (r.delays * 1000);
	r.delayns = delay * 1000 * 1000;
}

void WaylandKeyboardContext::repeatKey() {
	std::uint64_t val;
	if(read(timerfd_, &val, 8) != 8) {
		return;
	}

	Keycode keycode;
	std::string utf8;

	XkbKeyboardContext::handleKey(repeatKey_ + 8, true, keycode, utf8);
	if(focus_) {
		KeyEvent ke;
		ke.eventData = nullptr;
		ke.keycode = keycode;
		ke.utf8 = utf8;
		ke.pressed = true;
		ke.repeat = true;
		focus_->listener().key(ke);
	}
}

void WaylandKeyboardContext::destroyed(const WaylandWindowContext& wc) {
	if(focus_ == &wc) {
		focus_ = nullptr;
	}
}

} // namespace ny
