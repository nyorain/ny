// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/android/input.hpp>
#include <ny/android/appContext.hpp>
#include <ny/android/windowContext.hpp>
#include <ny/mouseButton.hpp>
#include <ny/event.hpp>
#include <ny/key.hpp>

#include <nytl/vec.hpp>

#include <android/keycodes.h>
#include <android/input.h>

namespace ny {
namespace {

constexpr struct {
	unsigned int android;
	Keycode keycode;
} keyMappings[] {
	{AKEYCODE_0, Keycode::k0},
	{AKEYCODE_1, Keycode::k1},
	{AKEYCODE_2, Keycode::k2},
	{AKEYCODE_3, Keycode::k3},
	{AKEYCODE_4, Keycode::k4},
	{AKEYCODE_5, Keycode::k5},
	{AKEYCODE_6, Keycode::k6},
	{AKEYCODE_7, Keycode::k7},
	{AKEYCODE_8, Keycode::k8},
	{AKEYCODE_9, Keycode::k9},

	{AKEYCODE_A, Keycode::a},
	{AKEYCODE_B, Keycode::b},
	{AKEYCODE_C, Keycode::c},
	{AKEYCODE_D, Keycode::d},
	{AKEYCODE_E, Keycode::e},
	{AKEYCODE_F, Keycode::f},
	{AKEYCODE_G, Keycode::g},
	{AKEYCODE_H, Keycode::h},
	{AKEYCODE_I, Keycode::i},
	{AKEYCODE_J, Keycode::j},
	{AKEYCODE_K, Keycode::k},
	{AKEYCODE_L, Keycode::l},
	{AKEYCODE_M, Keycode::m},
	{AKEYCODE_N, Keycode::n},
	{AKEYCODE_O, Keycode::o},
	{AKEYCODE_P, Keycode::p},
	{AKEYCODE_Q, Keycode::q},
	{AKEYCODE_R, Keycode::r},
	{AKEYCODE_S, Keycode::s},
	{AKEYCODE_T, Keycode::t},
	{AKEYCODE_U, Keycode::u},
	{AKEYCODE_V, Keycode::v},
	{AKEYCODE_W, Keycode::w},
	{AKEYCODE_X, Keycode::x},
	{AKEYCODE_Y, Keycode::y},
	{AKEYCODE_Z, Keycode::z},

	{AKEYCODE_COMMA, Keycode::comma},
	{AKEYCODE_PERIOD, Keycode::period},
	{AKEYCODE_ALT_LEFT, Keycode::leftalt},
	{AKEYCODE_ALT_RIGHT, Keycode::rightalt},
	{AKEYCODE_SHIFT_RIGHT, Keycode::rightshift},
	{AKEYCODE_SHIFT_LEFT, Keycode::leftshift},
	{AKEYCODE_TAB, Keycode::tab},
	{AKEYCODE_SPACE, Keycode::space},

	// ...

	{AKEYCODE_ENTER, Keycode::enter},
	{AKEYCODE_DEL, Keycode::del},
	{AKEYCODE_GRAVE, Keycode::grave},
	{AKEYCODE_MINUS, Keycode::minus},
	{AKEYCODE_EQUALS, Keycode::equals},
	{AKEYCODE_LEFT_BRACKET, Keycode::leftbrace},
	{AKEYCODE_RIGHT_BRACKET, Keycode::rightbrace},
	{AKEYCODE_BACKSLASH, Keycode::backslash},
	{AKEYCODE_SEMICOLON, Keycode::semicolon},
	{AKEYCODE_APOSTROPHE, Keycode::apostrophe},
	{AKEYCODE_SLASH, Keycode::slash},

	// ...

	{AKEYCODE_ESCAPE, Keycode::escape},
	{AKEYCODE_CTRL_LEFT, Keycode::leftctrl},
	{AKEYCODE_CTRL_RIGHT, Keycode::rightctrl},
	{AKEYCODE_SCROLL_LOCK, Keycode::scrollock},
	{AKEYCODE_META_LEFT, Keycode::leftmeta},
	{AKEYCODE_META_RIGHT, Keycode::rightmeta},

	// ...

	{AKEYCODE_MEDIA_PLAY, Keycode::play},
	{AKEYCODE_MEDIA_PAUSE, Keycode::pause},
	{AKEYCODE_MEDIA_CLOSE, Keycode::close},

	// ...

	{AKEYCODE_F1, Keycode::f1},
	{AKEYCODE_F2, Keycode::f2},
	{AKEYCODE_F3, Keycode::f3},
	{AKEYCODE_F4, Keycode::f4},
	{AKEYCODE_F5, Keycode::f5},
	{AKEYCODE_F6, Keycode::f6},
	{AKEYCODE_F7, Keycode::f7},
	{AKEYCODE_F9, Keycode::f8},
	{AKEYCODE_F10, Keycode::f10},
	{AKEYCODE_F11, Keycode::f11},
	{AKEYCODE_F12, Keycode::f12},

	{AKEYCODE_NUM_LOCK, Keycode::numlock},
	{AKEYCODE_NUMPAD_0, Keycode::kp0},
	{AKEYCODE_NUMPAD_1, Keycode::kp1},
	{AKEYCODE_NUMPAD_2, Keycode::kp2},
	{AKEYCODE_NUMPAD_3, Keycode::kp3},
	{AKEYCODE_NUMPAD_4, Keycode::kp4},
	{AKEYCODE_NUMPAD_5, Keycode::kp5},
	{AKEYCODE_NUMPAD_6, Keycode::kp6},
	{AKEYCODE_NUMPAD_7, Keycode::kp7},
	{AKEYCODE_NUMPAD_8, Keycode::kp8},
	{AKEYCODE_NUMPAD_9, Keycode::kp9},
	{AKEYCODE_NUMPAD_DIVIDE, Keycode::kpdivide},
	{AKEYCODE_NUMPAD_MULTIPLY, Keycode::kpmultiply},
	{AKEYCODE_NUMPAD_SUBTRACT, Keycode::kpminus},
	{AKEYCODE_NUMPAD_ADD, Keycode::kpplus},
	{AKEYCODE_NUMPAD_DOT, Keycode::kpperiod},
	{AKEYCODE_NUMPAD_COMMA, Keycode::kpcomma},
	{AKEYCODE_NUMPAD_ENTER, Keycode::kpenter},
	{AKEYCODE_NUMPAD_EQUALS, Keycode::kpequals},
};

constexpr struct {
	unsigned int android;
	KeyboardModifier modifier;
} modifierMappings[] = {
	{AMETA_ALT_ON, KeyboardModifier::alt},
	{AMETA_SHIFT_ON, KeyboardModifier::shift},
	{AMETA_CTRL_ON, KeyboardModifier::ctrl},
	{AMETA_META_ON, KeyboardModifier::super},
	{AMETA_CAPS_LOCK_ON, KeyboardModifier::capsLock},
	{AMETA_NUM_LOCK_ON, KeyboardModifier::numLock}
};

} // anonymous util namespace

// util functions
Keycode androidToKeycode(unsigned int code)
{
	for(const auto& mapping : keyMappings) {
		if(mapping.android == code)
			return mapping.keycode;
	}

	return Keycode::none;
}

unsigned int keycodeToAndroid(Keycode keycode)
{
	for(const auto& mapping : keyMappings) {
		if(mapping.keycode == keycode)
			return mapping.android;
	}

	return AKEYCODE_UNKNOWN;
}

KeyboardModifiers androidToModifiers(unsigned int mask)
{
	KeyboardModifiers ret {};
	for(const auto& mapping : modifierMappings) {
		if(mask & mapping.android)
			ret |= mapping.modifier;
	}

	return ret;
}

unsigned int modifiersToAndroid(KeyboardModifiers mask)
{
	unsigned int ret {};
	for(const auto& mapping : modifierMappings) {
		if(mask & mapping.modifier)
			ret |= mapping.android;
	}

	return ret;
}

// KeyboardContext
bool AndroidKeyboardContext::pressed(Keycode key) const
{
	auto keyInt = static_cast<unsigned int>(key);
	if(keyInt >= 255)
		return false;

	return keyStates_[keyInt];
}

WindowContext* AndroidKeyboardContext::focus() const
{
	return appContext_.windowContext();
}

KeyboardModifiers AndroidKeyboardContext::modifiers() const
{
	return modifiers_;
}

std::string AndroidKeyboardContext::utf8(Keycode) const
{
	// TODO: can be done using JniEnv (in process function as well)
	return "";
}

bool AndroidKeyboardContext::process(const AInputEvent& event)
{
	auto wc = appContext_.windowContext();

	AndroidEventData eventData;
	eventData.inputEvent = &event;

	auto action = AKeyEvent_getAction(&event);
	auto metaState = AKeyEvent_getMetaState(&event);
	modifiers_ = androidToModifiers(metaState);

	auto akeycode = AKeyEvent_getKeyCode(&event);

	// we skip this keycode so that is handled by android
	// the application can't handle it anyways (at the moment - TODO?!)
	if(akeycode == AKEYCODE_BACK)
		return false;

	if(action == AKEY_EVENT_ACTION_DOWN || action == AKEY_EVENT_ACTION_UP) {
		bool pressed = (action == AKEY_EVENT_ACTION_DOWN);
		auto keycode = androidToKeycode(akeycode);

		// skip button we don't know
		// TODO: merge this with back keycode check?
		if(keycode == Keycode::none)
			return false;

		std::string utf8 = ""; // TODO! can be done using JniEnv

		if(wc) {
			KeyEvent keyEvent;
			keyEvent.pressed = pressed;
			keyEvent.keycode = keycode;
			keyEvent.utf8 = utf8;
			keyEvent.modifiers = modifiers_;
			keyEvent.eventData = &eventData;
			wc->listener().key(keyEvent);
		}

		auto intCode = static_cast<unsigned int>(keycode);
		keyStates_[intCode] = pressed;
		onKey(*this, keycode, utf8, pressed);

		return true;
	}

	return false;
}

// MouseContext [or rather touch context]
// TODO!
nytl::Vec2i AndroidMouseContext::position() const
{
	return {};
}
bool AndroidMouseContext::pressed(MouseButton) const
{
	return false;
}
WindowContext* AndroidMouseContext::over() const
{
	return appContext_.windowContext();
}

bool AndroidMouseContext::process(const AInputEvent& event)
{
	// TODO!
	if(!appContext_.windowContext())
		return false;

	auto& wc = *appContext_.windowContext();
	AndroidEventData eventData;
	eventData.inputEvent = &event;

	// TODO: query more than 1 pointer event
	// auto pointerCount = AMotionEvent_getPointerCount(&event);

	// query general information
	nytl::Vec2i pos;
	pos.x = AMotionEvent_getX(&event, 0);
	pos.y = AMotionEvent_getY(&event, 0);

	// TODO: handle real buttons
	// TODO: call callbacks

	// switch the actions
	auto action = AMotionEvent_getAction(&event);
	switch(action) {
		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_POINTER_DOWN: {
			MouseButtonEvent mbe;
			mbe.position = pos;
			mbe.eventData = &eventData;
			mbe.button = MouseButton::left;
			mbe.pressed = true;
			wc.listener().mouseButton(mbe);
			break;
		} case AMOTION_EVENT_ACTION_POINTER_UP:
		case AMOTION_EVENT_ACTION_UP: {
			MouseButtonEvent mbe;
			mbe.position = pos;
			mbe.eventData = &eventData;
			mbe.button = MouseButton::left;
			mbe.pressed = false;
			wc.listener().mouseButton(mbe);
			break;
		} case AMOTION_EVENT_ACTION_MOVE: {
			MouseMoveEvent mme;
			mme.position = pos;
			mme.eventData = &eventData;
			wc.listener().mouseMove(mme);
			break;
		} case AMOTION_EVENT_ACTION_CANCEL: {
			// TODO
			break;
		} case AMOTION_EVENT_ACTION_HOVER_MOVE: {
			MouseMoveEvent mme;
			mme.position = pos;
			mme.eventData = &eventData;
			wc.listener().mouseMove(mme);
			break;
		} case AMOTION_EVENT_ACTION_SCROLL: {
			MouseWheelEvent mwe;
			mwe.eventData = &eventData;
			mwe.value = AMotionEvent_getAxisValue(&event, AMOTION_EVENT_AXIS_VSCROLL, 0);
			wc.listener().mouseWheel(mwe);
			break;
		} case AMOTION_EVENT_ACTION_HOVER_ENTER: {
			MouseCrossEvent mce;
			mce.entered = true;
			wc.listener().mouseCross(mce);
			break;
		} case AMOTION_EVENT_ACTION_HOVER_EXIT: {
			MouseCrossEvent mce;
			mce.entered = true;
			wc.listener().mouseCross(mce);
			break;
		} default:
			return false;
	}

	return true;
}

} // namespace ny
