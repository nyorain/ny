// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/input.hpp>
#include <ny/winapi/windows.hpp>
#include <ny/winapi/util.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/log.hpp>
#include <ny/mouseButton.hpp>
#include <nytl/utf.hpp>

namespace ny
{

Vec2i WinapiMouseContext::position() const
{
	if(!over_) return {};

	POINT p;
	if(!::GetCursorPos(&p)) return {};
	if(!::ScreenToClient(over_->handle(), &p)) return {};

	return nytl::Vec2i(p.x, p.y);
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

	using MB = MouseButton;
	auto handleMouseButton = [&](bool pressed, MouseButton button) {
		if(wc) wc->listener().mouseButton(pressed, button, &eventData);
		onButton(*this, button, pressed);
	};

	switch(message)
	{
		case WM_MOUSEMOVE:
		{
			Vec2i pos(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));

			// POINT screenPos {pos.x, pos.y};
			// ::ClientToScreen(window, &screenPos);

			//check for implicit mouse over change
			//windows does not send any mouse enter events, we have to detect them this way
			if(wc && over() != wc)
			{
				wc->listener().mouseCross(true, &eventData);

				//Request wm_mouseleave events
				//we have to do this everytime
				TRACKMOUSEEVENT trackMouse {};
				trackMouse.cbSize = sizeof(trackMouse);
				trackMouse.dwFlags = TME_LEAVE;
				trackMouse.hwndTrack = window;
				::TrackMouseEvent(&trackMouse);

				onFocus(*this, over_, wc);
				over_ = wc;
			}

			if(wc) wc->listener().mouseMove(position_, &eventData);

			auto delta = pos - position_;
			position_ = pos;
			onMove(*this, pos, delta);

			break;
		}

		case WM_MOUSELEAVE:
		{
			if(over_)
			{
				over_->listener().mouseCross(false, &eventData);

				onFocus(*this, over_, nullptr);
				over_ = nullptr;
			}

			break;
		}

		case WM_LBUTTONDOWN: handleMouseButton(true, MB::left); break;
		case WM_LBUTTONUP: handleMouseButton(false, MB::left); break;
		case WM_RBUTTONDOWN: handleMouseButton(true, MB::right); break;
		case WM_RBUTTONUP: handleMouseButton(false, MB::right); break;
		case WM_MBUTTONDOWN: handleMouseButton(true, MB::middle); break;
		case WM_MBUTTONUP: handleMouseButton(false, MB::middle); break;
		case WM_XBUTTONDOWN:
		{
			auto button = (HIWORD(wparam) == 1) ? MB::custom1 : MB::custom2;
			handleMouseButton(true, button);
			break;
		}

		case WM_XBUTTONUP:
		{

			auto button = (HIWORD(wparam) == 1) ? MB::custom1 : MB::custom2;
			handleMouseButton(false, button);
			break;
		}

		case WM_MOUSEWHEEL:
		{
			float value = GET_WHEEL_DELTA_WPARAM(wparam);
			if(wc) wc->listener().mouseWheel(value, &eventData);
			onWheel(*this, value);
			break;
		}

		default: return false;
	}

	result = 0;
	return true;
}


//KeyboardContext
WinapiKeyboardContext::WinapiKeyboardContext(WinapiAppContext& context) : context_(context)
{
	wchar_t unicode[16] {};
	unsigned char state[256] {};
	auto decimalScancode = ::MapVirtualKey(VK_DECIMAL, MAPVK_VK_TO_VSC);

	for(auto i = 0u; i < 255u; ++i)
	{
		auto keycode = static_cast<Keycode>(i);
		auto vkcode = keycodeToWinapi(keycode);
		auto scancode = ::MapVirtualKey(vkcode, MAPVK_VK_TO_VSC);

		//first reset the current keyboard buffer state regarding dead keys
		int ret;
		do { ret = ::ToUnicode(VK_DECIMAL, decimalScancode, state, unicode, 16, 0); }
		while(ret < 0);

		//now read the real keycode
		ret = ::ToUnicode(vkcode, scancode, state, unicode, 16, 0);
		if(ret <= 0) continue;

		unicode[ret] = L'\0';
		auto utf16string = reinterpret_cast<char16_t*>(unicode);
		keycodeUnicodeMap_[keycode] = nytl::toUtf8(utf16string);
	}
}

bool WinapiKeyboardContext::pressed(Keycode key) const
{
	auto keyState = ::GetAsyncKeyState(keycodeToWinapi(key));
	return ((1 << 16) & keyState);
}

std::string WinapiKeyboardContext::utf8(Keycode keycode) const
{
	auto it = keycodeUnicodeMap_.find(keycode);
	if(it != keycodeUnicodeMap_.end()) return it->second;
	return "";
}

bool WinapiKeyboardContext::processEvent(const WinapiEventData& eventData, LRESULT& result)
{
	auto message = eventData.message;
	auto wc = eventData.windowContext;
	auto lparam = eventData.lparam;
	auto wparam = eventData.wparam;

	bool keyPressed = false;

	switch(message)
	{
		case WM_SETFOCUS:
		{
			if(wc && wc != focus_)
			{
				wc->listener().focus(true, &eventData);
				onFocus(*this, focus_, wc);
				focus_ = wc;
			}

			break;
		}

		case WM_KILLFOCUS:
		{
			if(wc && focus_ == wc)
			{
				wc->listener().focus(false, &eventData);
				onFocus(*this, focus_, nullptr);
				focus_ = nullptr;
			}

			break;
		}

		case WM_KEYDOWN: keyPressed = true;
		case WM_KEYUP:
		{
			auto vkcode = wparam;
			auto scancode = HIWORD(lparam);
			auto keycode = winapiToKeycode(vkcode);

			unsigned char state[256] {};
			::GetKeyboardState(state);

			std::string utf8;
			wchar_t utf16[64];
			auto bytes = ::ToUnicode(vkcode, scancode, state, utf16, 64, 0);
			if(bytes > 0)
			{
				utf16[bytes] = L'\0';
				auto utf16string = reinterpret_cast<char16_t*>(utf16);
				utf8 = nytl::toUtf8(utf16string);
			}

			if(wc) wc->listener().key(keyPressed, keycode, utf8, &eventData);
			onKey(*this, keycode, utf8, keyPressed);

			break;
		}

		default: return false;
	}

	result = 0;
	return true;
}

}
