// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/input.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/windowContext.hpp>
#include <ny/x11/util.hpp>
#include <dlg/dlg.hpp>

#include <nytl/utf.hpp>
#include <nytl/vecOps.hpp>

#include <xcb/xcb.h>

// i'm really sorry for this...
// the xkb header is not very c++ friendly
#define explicit explicit_
	#include <xcb/xkb.h>
#undef explicit

#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon-compose.h>

namespace ny {

// MouseContext
bool X11MouseContext::processEvent(const x11::GenericEvent& ev)
{
	X11EventData eventData {ev};

	auto responseType = ev.response_type & ~0x80;
	switch(responseType) {
		case XCB_MOTION_NOTIFY: {
			auto& motion = reinterpret_cast<const xcb_motion_notify_event_t&>(ev);
			auto pos = nytl::Vec2i{motion.event_x, motion.event_y};

			if(pos == lastPosition_) break;
			auto delta = pos - lastPosition_;
			onMove(*this, pos, delta);
			lastPosition_ = pos;

			auto wc = appContext().windowContext(motion.event);

			if(wc) {
				MouseMoveEvent mme;
				mme.eventData = &eventData;
				mme.position = pos;
				mme.delta = delta;
				wc->listener().mouseMove(mme);
			}

			break;
		}

		case XCB_BUTTON_PRESS: {
			auto& button = reinterpret_cast<const xcb_button_press_event_t&>(ev);

			int scroll = 0;
			if(button.detail == 4) scroll = 1;
			else if(button.detail == 5) scroll = -1;

			auto wc = appContext().windowContext(button.event);
			auto pos = nytl::Vec2i{button.event_x, button.event_y};

			if(scroll) {
				MouseWheelEvent mwe;
				mwe.eventData = &eventData;
				mwe.value = scroll;
				mwe.position = pos;
				if(wc) wc->listener().mouseWheel(mwe);
				onWheel(*this, scroll);
				break;
			}

			auto nybutton = x11ToButton(button.detail);
			buttonStates_[static_cast<unsigned int>(nybutton)] = true;
			onButton(*this, nybutton, true);

			if(wc) {
				MouseButtonEvent mbe;
				mbe.pressed = true;
				mbe.position = pos;
				mbe.button = nybutton;
				mbe.eventData = &eventData;
				wc->listener().mouseButton(mbe);
			}

			break;
		}

		case XCB_BUTTON_RELEASE: {
			auto& button = reinterpret_cast<const xcb_button_release_event_t&>(ev);
			if(button.detail == 4 || button.detail == 5) break;

			auto nybutton = x11ToButton(button.detail);
			buttonStates_[static_cast<unsigned int>(nybutton)] = false;
			onButton(*this, nybutton, false);

			auto wc = appContext().windowContext(button.event);
			if(wc) {
				auto pos = nytl::Vec2i{button.event_x, button.event_y};
				MouseButtonEvent mbe;
				mbe.pressed = false;
				mbe.position = pos;
				mbe.button = nybutton;
				mbe.eventData = &eventData;
				wc->listener().mouseButton(mbe);
			}
			break;
		}

		case XCB_ENTER_NOTIFY: {
			auto& enter = reinterpret_cast<const xcb_enter_notify_event_t&>(ev);

			auto wc = appContext().windowContext(enter.event);
			if(over_ != wc) {
				onFocus(*this, over_, wc);
				over_ = wc;
			}

			if(wc) {
				auto pos = nytl::Vec2i{enter.event_x, enter.event_y};
				MouseCrossEvent mce;
				mce.eventData = &eventData;
				mce.entered = true;
				mce.position = pos;
				wc->listener().mouseCross(mce);
			}

			break;
		}

		case XCB_LEAVE_NOTIFY: {
			auto& leave = reinterpret_cast<const xcb_enter_notify_event_t&>(ev);

			auto wc = appContext().windowContext(leave.event);
			if(over_ == wc) {
				onFocus(*this, over_, nullptr);
				over_ = nullptr;
			}

			if(wc) {
				auto pos = nytl::Vec2i{leave.event_x, leave.event_y};
				MouseCrossEvent mce;
				mce.eventData = &eventData;
				mce.entered = false;
				mce.position = pos;
				wc->listener().mouseCross(mce);
			}

			break;
		}

		default: return false;
	}

	return true;
}

nytl::Vec2i X11MouseContext::position() const
{
	if(!over_) return {};

	xcb_generic_error_t* error {};
	auto cookie = xcb_query_pointer(&appContext_.xConnection(), over_->xWindow());
	auto reply = xcb_query_pointer_reply(&appContext_.xConnection(), cookie, &error);

	if(error) {
		auto msg = x11::errorMessage(appContext_.xDisplay(), error->error_code);
		dlg_warn("xcb_query_pointer: {}", msg);
		free(error);
		return {};
	}

	auto pos = nytl::Vec2i{reply->win_x, reply->win_y};
	free(reply);
	return pos;
}

bool X11MouseContext::pressed(MouseButton button) const
{
	// TODO: async checking?
	return buttonStates_[static_cast<unsigned int>(button)];
}

WindowContext* X11MouseContext::over() const
{
	return over_;
}

// Keyboard
X11KeyboardContext::X11KeyboardContext(X11AppContext& ac) : appContext_(ac)
{
	// TODO: possibility for backend without xkb (if extension not supported?)
	// more error checking required
	auto& xconn = appContext_.xConnection();

	xkbContext_ = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if(!xkbContext_) throw std::runtime_error("ny::X11KC: failed to create xkb_context");

	std::uint16_t major, minor;
	auto ret = xkb_x11_setup_xkb_extension(&xconn, XKB_X11_MIN_MAJOR_XKB_VERSION,
		XKB_X11_MIN_MINOR_XKB_VERSION, XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
		&major, &minor, &eventType_, nullptr);
	if(!ret) throw std::runtime_error("X11KC: Failed to setup xkb extension");
	// dlg_debug("xkb version {}.{}", major, minor);

	auto devid = xkb_x11_get_core_keyboard_device_id(&xconn);
	auto flags = XKB_KEYMAP_COMPILE_NO_FLAGS;
	xkbKeymap_ = xkb_x11_keymap_new_from_device(xkbContext_, &xconn, devid, flags);
	xkbState_ =  xkb_x11_state_new_from_device(xkbKeymap_, &xconn, devid);

	// event mask
	constexpr auto reqEvents =
		XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
		XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
		XCB_XKB_EVENT_TYPE_STATE_NOTIFY;
	constexpr auto reqNknDetails = XCB_XKB_NKN_DETAIL_KEYCODES;
	constexpr auto reqMapParts =
		XCB_XKB_MAP_PART_KEY_TYPES |
		XCB_XKB_MAP_PART_KEY_SYMS |
		XCB_XKB_MAP_PART_MODIFIER_MAP |
		XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
		XCB_XKB_MAP_PART_KEY_ACTIONS |
		XCB_XKB_MAP_PART_VIRTUAL_MODS |
		XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP;
	constexpr auto reqStateDetails =
		XCB_XKB_STATE_PART_MODIFIER_BASE |
		XCB_XKB_STATE_PART_MODIFIER_LATCH |
		XCB_XKB_STATE_PART_MODIFIER_LOCK |
		XCB_XKB_STATE_PART_GROUP_BASE |
		XCB_XKB_STATE_PART_GROUP_LATCH |
		XCB_XKB_STATE_PART_GROUP_LOCK;

	xcb_xkb_select_events_details_t details {};
	details.affectNewKeyboard = reqNknDetails;
	details.newKeyboardDetails = reqNknDetails;
	details.affectState = reqStateDetails;
	details.stateDetails = reqStateDetails;

	auto cookie = xcb_xkb_select_events_aux_checked(&xconn, devid, reqEvents, 0, 0,
		reqMapParts, reqMapParts, &details);

	auto error = xcb_request_check(&xconn, cookie);
	if(error) {
		free(error);
		auto msg = "x11kc: failed to select xkb events: " + std::to_string((int) error->error_code);
		throw std::runtime_error(msg);
	}

	XkbKeyboardContext::setupCompose();
}

bool X11KeyboardContext::pressed(Keycode key) const
{
	// TODO: XQueryKeymap for async checking
	auto uint = static_cast<unsigned int>(key);
	if(uint > 255) return false;
	return keyStates_[uint];
}

WindowContext* X11KeyboardContext::focus() const
{
	return focus_;
}

bool X11KeyboardContext::processEvent(const x11::GenericEvent& ev, const x11::GenericEvent* next)
{
	union XkbEvent {
		struct {
			std::uint8_t response_type;
			std::uint8_t xkbType;
			std::uint16_t sequence;
			xcb_timestamp_t time;
			uint8_t deviceID;
		} any;

		xcb_xkb_new_keyboard_notify_event_t newKeyboard;
		xcb_xkb_map_notify_event_t map;
		xcb_xkb_state_notify_event_t state;
	};

	X11EventData eventData {ev};

	auto responseType = ev.response_type & ~0x80;
	switch(responseType) {
		case XCB_FOCUS_IN: {
			auto& focus = reinterpret_cast<const xcb_focus_in_event_t&>(ev);
			auto wc = appContext().windowContext(focus.event);

			if(focus_ != wc) {
				onFocus(*this, focus_, wc);
				focus_ = wc;
			}

			if(wc) {
				FocusEvent fe;
				fe.eventData = &eventData;
				fe.gained = true;
				wc->listener().focus(fe);
			}

			break;
		}

		case XCB_FOCUS_OUT: {
			auto& focus = reinterpret_cast<const xcb_focus_in_event_t&>(ev);
			auto wc = appContext().windowContext(focus.event);

			if(focus_ == wc) {
				onFocus(*this, focus_, nullptr);
				focus_ = nullptr;
			}

			if(wc) {
				FocusEvent fe;
				fe.eventData = &eventData;
				fe.gained = false;
				wc->listener().focus(fe);
			}

			break;
		}

		case XCB_KEY_PRESS: {
			auto& key = reinterpret_cast<const xcb_key_press_event_t&>(ev);
			auto wc = appContext().windowContext(key.event);

			Keycode keycode;
			std::string utf8;

			// When the user presses keys that cancel a dead key, we ring the bell.
			if(!handleKey(key.detail, true, keycode, utf8)) {
				appContext().bell();
			}
			
			onKey(*this, keycode, utf8, true);

			if(wc) {
				KeyEvent ke;
				ke.eventData = &eventData;
				ke.keycode = keycode;
				ke.utf8 = utf8;
				ke.pressed = true;
				ke.repeat = repeated_;
				wc->listener().key(ke);
			}

			repeated_ = false;
			break;
		}

		case XCB_KEY_RELEASE: {
			auto& key = reinterpret_cast<const xcb_key_press_event_t&>(ev);
			auto wc = appContext().windowContext(key.event);

			Keycode keycode;
			std::string utf8;

			// check for repeat
			if(next && (next->response_type & ~0x80) == XCB_KEY_PRESS) {
				auto& kp = reinterpret_cast<const xcb_key_press_event_t&>(*next);
				if(kp.time == key.time && kp.detail == key.detail) {
					// just ignore this event
					repeated_ = true;
					break;
				}
			}

			// When the user presses keys that cancel a dead key, we ring the bell.
			if(!handleKey(key.detail, false, keycode, utf8)) {
				appContext().bell();
			}

			onKey(*this, keycode, utf8, false);

			if(wc) {
				KeyEvent ke;
				ke.eventData = &eventData;
				ke.keycode = keycode;
				ke.utf8 = utf8;
				ke.pressed = false;
				wc->listener().key(ke);
			}

			break;
		}

		default: {
			if(ev.response_type == xkbEventType()) {
				auto& xkbev = reinterpret_cast<const XkbEvent&>(ev);
				switch(xkbev.any.xkbType) {
					case XCB_XKB_STATE_NOTIFY: {
						xkb_state_update_mask(xkbState_,
							xkbev.state.baseMods,
							xkbev.state.latchedMods,
							xkbev.state.lockedMods,
							xkbev.state.baseGroup,
							xkbev.state.latchedGroup,
							xkbev.state.lockedGroup);
						return true;
					}

					default: break;
				}
			}

			return false;
		}
	}

	return true;
}

bool X11KeyboardContext::updateKeymap()
{
	// TODO
	return true;
}

} // namespace ny
