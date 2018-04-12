// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/input.hpp>
#include <ny/winapi/windows.hpp>
#include <ny/winapi/util.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/key.hpp>
#include <ny/mouseButton.hpp>
#include <nytl/utf.hpp>
#include <dlg/dlg.hpp>

namespace ny {

nytl::Vec2i WinapiMouseContext::position() const
{
	if(!over_) return {};

	POINT p;
	if(!::GetCursorPos(&p)) return {};
	if(!::ScreenToClient(over_->handle(), &p)) return {};

	return {p.x, p.y};
}

bool WinapiMouseContext::pressed(MouseButton button) const
{
	auto keyState = ::GetAsyncKeyState(buttonToWinapi(button));
	return ((1 << 16) & keyState);
}

bool WinapiMouseContext::processEvent(const WinapiEventData& eventData, LRESULT& result)
{
	auto message = eventData.message;
	auto wc = eventData.windowContext;
	auto window = eventData.window;
	auto lparam = eventData.lparam;
	auto wparam = eventData.wparam;

	if(!wc) {
		return false;
	}

	using MB = MouseButton;
	auto handleMouseButton = [&](bool pressed, MouseButton button) {
		MouseButtonEvent mbe;
		mbe.eventData = &eventData;
		mbe.position = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
		mbe.button = button;
		mbe.pressed = pressed;
		wc->listener().mouseButton(mbe);
		onButton(*this, button, pressed);
	};

	switch(message) {
		case WM_MOUSEMOVE: {
			nytl::Vec2i pos{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};

			// check for implicit mouse over change
			// windows does not send any mouse enter events, we have to detect them this way
			if(wc && over() != wc) {
				MouseCrossEvent mce;
				mce.entered = true;
				mce.eventData = &eventData;
				mce.position = pos;
				wc->listener().mouseCross(mce);

				// Request wm_mouseleave events
				// we have to do this everytime
				// therefore we do not send leave event here (should be generated)
				TRACKMOUSEEVENT trackMouse {};
				trackMouse.cbSize = sizeof(trackMouse);
				trackMouse.dwFlags = TME_LEAVE;
				trackMouse.hwndTrack = window;
				::TrackMouseEvent(&trackMouse);

				onFocus(*this, over_, wc);
				over_ = wc;
			}

			MouseMoveEvent mme;
			mme.position = pos;
			mme.delta = pos - position_;
			mme.eventData = &eventData;
			wc->listener().mouseMove(mme);

			position_ = pos;
			onMove(*this, pos, mme.delta);
			break;
		}

		case WM_MOUSELEAVE: {
			MouseCrossEvent mce;
			mce.eventData = &eventData;
			mce.entered = false;
			mce.position = position_;
			wc->listener().mouseCross(mce);

			if(wc == over_) onFocus(*this, over_, nullptr);
			over_ = nullptr;
			break;
		}

		case WM_LBUTTONDOWN: handleMouseButton(true, MB::left); break;
		case WM_LBUTTONUP: handleMouseButton(false, MB::left); break;
		case WM_RBUTTONDOWN: handleMouseButton(true, MB::right); break;
		case WM_RBUTTONUP: handleMouseButton(false, MB::right); break;
		case WM_MBUTTONDOWN: handleMouseButton(true, MB::middle); break;
		case WM_MBUTTONUP: handleMouseButton(false, MB::middle); break;
		case WM_XBUTTONDOWN: {
			auto button = (HIWORD(wparam) == 1) ? MB::custom1 : MB::custom2;
			handleMouseButton(true, button);
			break;
		}

		case WM_XBUTTONUP: {
			auto button = (HIWORD(wparam) == 1) ? MB::custom1 : MB::custom2;
			handleMouseButton(false, button);
			break;
		}

		case WM_MOUSEWHEEL: {
			POINT screenPos {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
			::ClientToScreen(window, &screenPos);

			MouseWheelEvent mwe {};
			mwe.eventData = &eventData;
			mwe.value.y = GET_WHEEL_DELTA_WPARAM(wparam) / 120.0;
			mwe.position = {screenPos.x, screenPos.y};
			wc->listener().mouseWheel(mwe);
			onWheel(*this, mwe.value);
			break;
		}

		case WM_MOUSEHWHEEL: {
			POINT screenPos {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
			::ClientToScreen(window, &screenPos);

			MouseWheelEvent mwe {};
			mwe.eventData = &eventData;
			mwe.value.x = -GET_WHEEL_DELTA_WPARAM(wparam) / 120.0;
			mwe.position = {screenPos.x, screenPos.y};
			wc->listener().mouseWheel(mwe);
			onWheel(*this, mwe.value);
			break;
		}

		default: return false;
	}

	result = 0;
	return true;
}


// KeyboardContext
WinapiKeyboardContext::WinapiKeyboardContext(WinapiAppContext& context) : context_(context)
{
	// TODO: load them in an extra function,
	//  and store the current ::GetKeyboardLayout()
	//  in this.utf8() check ::GetKeyboardLayout() is still the same,
	//  otherwise refresh
	wchar_t unicode[16] {};
	unsigned char state[256] {};
	auto decimalScancode = ::MapVirtualKey(VK_DECIMAL, MAPVK_VK_TO_VSC);

	for(auto i = 0u; i < 255u; ++i) {
		auto keycode = static_cast<Keycode>(i);
		auto vkcode = keycodeToWinapi(keycode);
		auto scancode = ::MapVirtualKey(vkcode, MAPVK_VK_TO_VSC);

		// first reset the current keyboard buffer state regarding dead keys
		int ret;
		do {
			ret = ::ToUnicode(VK_DECIMAL, decimalScancode, state, unicode, 16, 0);
		} while(ret < 0);

		// now read the real keycode
		ret = ::ToUnicode(vkcode, scancode, state, unicode, 16, 0);
		if(ret <= 0) continue;

		unicode[ret] = L'\0';
		auto utf16string = reinterpret_cast<char16_t*>(unicode);
		keycodeUnicodeMap_[keycode] = nytl::toUtf8(utf16string);
	}
}

bool WinapiKeyboardContext::pressed(Keycode key) const
{
	return pressed(keycodeToWinapi(key));
}

std::string WinapiKeyboardContext::utf8(Keycode keycode) const
{
	// TODO: use GetKeyNameText ?
	auto it = keycodeUnicodeMap_.find(keycode);
	if(it != keycodeUnicodeMap_.end()) return it->second;
	return "";
}

KeyboardModifiers WinapiKeyboardContext::modifiers() const
{
	auto mods = KeyboardModifiers {};
	if(pressed(VK_SHIFT)) mods |= KeyboardModifier::shift;
	if(pressed(VK_CONTROL)) mods |= KeyboardModifier::ctrl;
	if(pressed(VK_MENU)) mods |= KeyboardModifier::alt;
	if(pressed(VK_LWIN) || pressed(VK_RWIN)) mods |= KeyboardModifier::super;
	if(pressed(VK_CAPITAL)) mods |= KeyboardModifier::capsLock;
	if(pressed(VK_NUMLOCK)) mods |= KeyboardModifier::numLock;
	return mods;
}

bool WinapiKeyboardContext::pressed(unsigned int vkcode) const
{
	auto keyState = ::GetAsyncKeyState(vkcode);
	return ((1 << 16) & keyState);
}

bool WinapiKeyboardContext::processEvent(const WinapiEventData& eventData, LRESULT& result)
{
	auto message = eventData.message;
	auto wc = eventData.windowContext;
	auto lparam = eventData.lparam;
	auto wparam = eventData.wparam;
	result = 0;

	bool keyPressed = false;
	if(!wc) {
		return false;
	}

	switch(message) {
		case WM_SETFOCUS: {
			if(wc != focus_) {
				FocusEvent fe;
				fe.eventData = &eventData;
				fe.gained = true;
				wc->listener().focus(fe);
				onFocus(*this, focus_, wc);
				focus_ = wc;
			}

			break;
		}

		case WM_KILLFOCUS: {
			if(focus_ == wc) {
				FocusEvent fe;
				fe.eventData = &eventData;
				fe.gained = false;
				wc->listener().focus(fe);
				onFocus(*this, focus_, nullptr);
				focus_ = nullptr;
			}

			break;
		}

		case WM_KEYDOWN:
			keyPressed = true;
			[[fallthrough]];
		case WM_KEYUP: {
			// TODO: better scancode (extended) handling,
			// see https://handmade.network/forums/t/2011-keyboard_inputs_-_scancodes,_raw_input,_text_input,_key_names
			auto vkcode = wparam;
			auto scancode = HIWORD(lparam);
			auto keycode = winapiToKeycode(vkcode);

			unsigned char state[256] {};
			::GetKeyboardState(state);

			KeyEvent ke;
			ke.keycode = keycode;
			ke.eventData = &eventData;
			ke.pressed = keyPressed;
			ke.modifiers = modifiers();
			ke.repeat = keyPressed && (lparam & 0x40000000);
			wchar_t utf16[64];
			auto bytes = ::ToUnicode(vkcode, scancode, state, utf16, 64, 0);
			if(bytes > 0) {
				utf16[bytes] = L'\0';
				auto utf16string = reinterpret_cast<char16_t*>(utf16);
				ke.utf8 = nytl::toUtf8(utf16string);
			}

			wc->listener().key(ke);
			onKey(*this, keycode, ke.utf8, keyPressed);

			break;
		}

		/*
		case WM_IME_CHAR: {
			dlg_info("ime char: {}", wparam);
			return false;
		}

		case WM_CHAR: {
			dlg_info("char: {}", wparam);
			return false;
		}
		*/

		default: return false;
	}

	return true;
}

} // namespace ny
