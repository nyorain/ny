// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/input.hpp>
#include <ny/winapi/windows.hpp>
#include <ny/winapi/util.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/log.hpp>
#include <nytl/utf.hpp>

namespace ny
{

Vec2ui WinapiMouseContext::position() const
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

void WinapiMouseContext::over(WinapiWindowContext* wc)
{
	if(wc == over_) return;

	onFocus(*this, over_, wc);
	over_ = wc;
}

nytl::Vec2i WinapiMouseContext::move(nytl::Vec2i pos)
{
	auto delta = pos - position_;
	position_ = pos;
	onMove(*this, pos, delta);

	return delta;
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
		ret = ::ToUnicode(vkcode, i, state, unicode, 16, 0);
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

void WinapiKeyboardContext::keyEvent(WinapiWindowContext* wc, unsigned int vkcode,
	unsigned int lparam)
{
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

	bool pressed = !(scancode & (1 << 15));
	onKey(*this, keycode, utf8, pressed);

	if(wc != focus_) warning("ny::WinapiKC::keyEvent: event handler <-> focus inconsistency");

	if(wc && wc->eventHandler())
	{
		KeyEvent keyEvent(wc->eventHandler());
		keyEvent.pressed = pressed;
		keyEvent.keycode = keycode;
		keyEvent.unicode = std::move(utf8);
		wc->eventHandler()->handleEvent(keyEvent);
	}
}

void WinapiKeyboardContext::focus(WinapiWindowContext* wc)
{
	if(wc == focus_) return;

	onFocus(*this, focus_, wc);
	focus_ = wc;
}

}
