// Copyright (c) 2016 nyorain
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

namespace ny
{

using namespace wayland;

//mouse
WaylandMouseContext::WaylandMouseContext(WaylandAppContext& ac, wl_seat& seat)
	: appContext_(ac)
{
	using WMC = WaylandMouseContext;
	constexpr static wl_pointer_listener listener = {
		memberCallback<decltype(&WMC::handleEnter), &WMC::handleEnter,
			void(wl_pointer*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t)>,
		memberCallback<decltype(&WMC::handleLeave), &WMC::handleLeave,
			void(wl_pointer*, uint32_t, wl_surface*)>,
		memberCallback<decltype(&WMC::handleMotion), &WMC::handleMotion,
			void(wl_pointer*, uint32_t, wl_fixed_t, wl_fixed_t)>,
		memberCallback<decltype(&WMC::handleButton), &WMC::handleButton,
			void(wl_pointer*, uint32_t, uint32_t, uint32_t, uint32_t)>,
		memberCallback<decltype(&WMC::handleAxis), &WMC::handleAxis,
			void(wl_pointer*, uint32_t, uint32_t, wl_fixed_t)>,
		memberCallback<decltype(&WMC::handleFrame), &WMC::handleFrame,
			void(wl_pointer*)>,
		memberCallback<decltype(&WMC::handleAxisSource), &WMC::handleAxisSource,
			void(wl_pointer*, uint32_t)>,
		memberCallback<decltype(&WMC::handleAxisStop), &WMC::handleAxisStop,
			void(wl_pointer*, uint32_t, uint32_t)>,
		memberCallback<decltype(&WMC::handleAxisDiscrete), &WMC::handleAxisDiscrete,
			void(wl_pointer*, uint32_t, int32_t)>,
	};

	wlPointer_ = wl_seat_get_pointer(&seat);
    wl_pointer_add_listener(wlPointer_, &listener, this);
	wlCursorSurface_ = wl_compositor_create_surface(&ac.wlCompositor());
}

WaylandMouseContext::~WaylandMouseContext()
{
	if(wlPointer_)
	{
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

void WaylandMouseContext::handleMotion(unsigned int time, wl_fixed_t x, wl_fixed_t y)
{
	nytl::unused(time);

	auto oldPos = position_;
	position_ = {wl_fixed_to_int(x), wl_fixed_to_int(y)};
	onMove(*this, position_, position_ - oldPos);

	if(over_) over_->listener().mouseMove(position_, nullptr);
}
void WaylandMouseContext::handleEnter(unsigned int serial, wl_surface* surface,
	wl_fixed_t x, wl_fixed_t y)
{
	nytl::Vec2i pos(wl_fixed_to_int(x), wl_fixed_to_int(y));

	position_ = pos;
	lastSerial_ = serial;
	cursorSerial_ = serial;

	auto wc = appContext_.windowContext(*surface);
	WaylandEventData eventData(serial);

	if(wc != over_)
	{
		onFocus(*this, over_, wc);

		//send leave event
		if(over_) over_->listener().mouseCross(false, &eventData);

		//if still in a ny window, send enter event and update cursor
		if(wc)
		{
			wc->listener().mouseCross(true, &eventData);
			cursorBuffer(wc->wlCursorBuffer(), wc->cursorHotspot(), wc->cursorSize());
		}

		over_ = wc;
	}
}
void WaylandMouseContext::handleLeave(unsigned int serial, wl_surface* surface)
{
	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	auto wc = appContext_.windowContext(*surface);
	if(wc != over_) warning("ny::WaylandMouseContext::handleLeave: over inconsistency");

	if(over_) onFocus(*this, over_, nullptr);
	if(wc) wc->listener().mouseCross(false, &eventData);

	over_ = nullptr;
}
void WaylandMouseContext::handleButton(unsigned int serial, unsigned int time,
	unsigned int button, bool pressed)
{
	nytl::unused(time);

	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	auto nybutton = linuxToButton(button);
	onButton(*this, nybutton, pressed);

	if(over_) over_->listener().mouseButton(pressed, nybutton, &eventData);
}

void WaylandMouseContext::handleAxis(unsigned int time, unsigned int axis, wl_fixed_t value)
{
	nytl::unused(time, axis);
	auto nvalue = wl_fixed_to_int(value);
	onWheel(*this, nvalue);

	if(over_) over_->listener().mouseWheel(nvalue, nullptr);
}

//Those events could be handled if more complex axis events are supported.
//Handling the fame event will only become useful when the 3 callbacks below are handled
void WaylandMouseContext::handleFrame()
{
}

void WaylandMouseContext::handleAxisSource(unsigned int source)
{
	nytl::unused(source);
}

void WaylandMouseContext::handleAxisStop(unsigned int time, unsigned int axis)
{
	nytl::unused(time, axis);
}

void WaylandMouseContext::handleAxisDiscrete(unsigned int axis, int discrete)
{
	nytl::unused(axis, discrete);
}

void WaylandMouseContext::cursorBuffer(wl_buffer* buf, nytl::Vec2i hs, nytl::Vec2ui size) const
{
	wl_pointer_set_cursor(wlPointer_, cursorSerial_, wlCursorSurface_, hs.x, hs.y);

	wl_surface_attach(wlCursorSurface_, buf, 0, 0);
	wl_surface_damage(wlCursorSurface_, 0, 0, size.x, size.y);
	wl_surface_commit(wlCursorSurface_);
}

//keyboard
WaylandKeyboardContext::WaylandKeyboardContext(WaylandAppContext& ac, wl_seat& seat)
	: appContext_(ac)
{
	using WKC = WaylandKeyboardContext;
	constexpr static wl_keyboard_listener listener = {
		memberCallback<decltype(&WKC::handleKeymap), &WKC::handleKeymap,
			void(wl_keyboard*, uint32_t, int32_t, uint32_t)>,
		memberCallback<decltype(&WKC::handleEnter), &WKC::handleEnter,
			void(wl_keyboard*, uint32_t, wl_surface*, wl_array*)>,
		memberCallback<decltype(&WKC::handleLeave), &WKC::handleLeave,
			void(wl_keyboard*, uint32_t, wl_surface*)>,
		memberCallback<decltype(&WKC::handleKey), &WKC::handleKey,
			void(wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t)>,
		memberCallback<decltype(&WKC::handleModifiers), &WKC::handleModifiers,
			void(wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>,
		memberCallback<decltype(&WKC::handleRepeatInfo), &WKC::handleRepeatInfo,
			void(wl_keyboard*, int32_t, int32_t)>,
	};

	wlKeyboard_ = wl_seat_get_keyboard(&seat);
    wl_keyboard_add_listener(wlKeyboard_, &listener, this);

	XkbKeyboardContext::createDefault();
	XkbKeyboardContext::setupCompose();
}

WaylandKeyboardContext::~WaylandKeyboardContext()
{
	if(wlKeyboard_)
	{
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
void WaylandKeyboardContext::handleKeymap(unsigned int format, int fd, unsigned int size)
{
	//always close the give fd
	auto fdGuard = nytl::makeScopeGuard([=]{ close(fd); });

	if(format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
	{
		log("WaylandKeyboardContext: invalid keymap format");
		return;
	}

	auto buf = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
	if(buf == MAP_FAILED)
	{
		log("WaylandKeyboardContext: cannot map keymap");
		return;
	}

	//always unmap the buffer
	auto mapGuard = nytl::makeScopeGuard([=]{ munmap(buf, size); });

	keymap_ = true;

	if(xkbState_) xkb_state_unref(xkbState_);
	if(xkbKeymap_) xkb_keymap_unref(xkbKeymap_);

	auto data = static_cast<char*>(buf);
	xkbKeymap_ = xkb_keymap_new_from_buffer(xkbContext_, data, size - 1,
		XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

	if(!xkbKeymap_)
	{
		log("WaylandKeyboardContext: failed to compile the xkb keymap from compositor.");
		return;
	}

	xkbState_ = xkb_state_new(xkbKeymap_);

	if(!xkbState_)
	{
		log("WaylandKeyboardContext: failed to create the xkbState from mapped keymap buffer");
		return;
	}
}
void WaylandKeyboardContext::handleEnter(unsigned int serial, wl_surface* surface, wl_array* keys)
{
	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	keyStates_.reset();
	for(auto i = 0u; i < keys->size / sizeof(std::uint32_t); ++i)
	{
		auto keyid = (static_cast<std::uint32_t*>(keys->data))[i];
		keyStates_[keyid] = true;
	}

	auto* wc = appContext_.windowContext(*surface);
	if(wc != focus_)
	{
		onFocus(*this, focus_, wc);

		if(focus_) focus_->listener().focus(false, &eventData);
		if(wc) wc->listener().focus(true, &eventData);

		focus_ = wc;
	}
}

void WaylandKeyboardContext::handleLeave(unsigned int serial, wl_surface* surface)
{
	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	auto* wc = appContext_.windowContext(*surface);
	if(wc != focus_) warning("ny::WaylandKeyboardContext::handleLeave: focus inconsistency");

	if(wc)
	{
		onFocus(*this, wc, nullptr);
		wc->listener().focus(false, &eventData);
	}

	keyStates_.reset();
	focus_ = nullptr;
}
void WaylandKeyboardContext::handleKey(unsigned int serial, unsigned int time,
	unsigned int key, bool pressed)
{
	nytl::unused(time);

	lastSerial_ = serial;
	WaylandEventData eventData(serial);

	Keycode keycode;
	std::string utf8;

	XkbKeyboardContext::handleKey(key + 8, pressed, keycode, utf8);
	if(focus_) focus_->listener().key(pressed, keycode, utf8, &eventData);
	onKey(*this, keycode, utf8, pressed);
}

void WaylandKeyboardContext::handleModifiers(unsigned int serial, unsigned int mdepressed,
	unsigned int mlatched, unsigned int mlocked, unsigned int group)
{
	lastSerial_ = serial;
	XkbKeyboardContext::updateState({mdepressed, mlatched, mlocked}, {group, group, group});
}

void WaylandKeyboardContext::handleRepeatInfo(int rate, int delay)
{
	nytl::unused(rate, delay);

	repeatRate_ = rate;
	repeatDelay_ = delay;
}

}
