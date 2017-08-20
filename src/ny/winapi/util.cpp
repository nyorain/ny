// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/util.hpp>
#include <ny/winapi/windows.hpp>
#include <ny/mouseContext.hpp>
#include <ny/cursor.hpp>
#include <ny/key.hpp>
#include <ny/log.hpp>
#include <ny/mouseButton.hpp>

#include <nytl/utf.hpp>
#include <nytl/scope.hpp>

namespace ny {

// maps ny::Keycode to a windows-specific virtual key code.
constexpr struct KeycodeConversion {
	Keycode keycode;
	unsigned int vkcode;
} keycodeConversions [] =
{
	{Keycode::none, 0x0},
	{Keycode::a, 'A'},
	{Keycode::b, 'B'},
	{Keycode::c, 'C'},
	{Keycode::d, 'D'},
	{Keycode::e, 'E'},
	{Keycode::f, 'F'},
	{Keycode::g, 'G'},
	{Keycode::h, 'H'},
	{Keycode::i, 'I'},
	{Keycode::j, 'J'},
	{Keycode::k, 'K'},
	{Keycode::l, 'L'},
	{Keycode::m, 'M'},
	{Keycode::n, 'N'},
	{Keycode::o, 'O'},
	{Keycode::p, 'P'},
	{Keycode::q, 'Q'},
	{Keycode::r, 'R'},
	{Keycode::s, 'S'},
	{Keycode::t, 'T'},
	{Keycode::u, 'U'},
	{Keycode::v, 'V'},
	{Keycode::w, 'W'},
	{Keycode::x, 'X'},
	{Keycode::y, 'Y'},
	{Keycode::z, 'Z'},
	{Keycode::k0, '0'},
	{Keycode::k1, '1'},
	{Keycode::k2, '2'},
	{Keycode::k3, '3'},
	{Keycode::k4, '4'},
	{Keycode::k5, '5'},
	{Keycode::k6, '6'},
	{Keycode::k7, '7'},
	{Keycode::k8, '8'},
	{Keycode::k9, '9'},
	{Keycode::backspace, VK_BACK},
	{Keycode::tab, VK_TAB},
	{Keycode::clear, VK_CLEAR},
	{Keycode::enter, VK_RETURN},
	{Keycode::leftshift, VK_SHIFT},
	{Keycode::leftctrl, VK_CONTROL},
	{Keycode::leftalt, VK_MENU},
	{Keycode::capslock, VK_CAPITAL},
	{Keycode::katakana, VK_KANA},
	{Keycode::hanguel, VK_HANGUL},
	{Keycode::hanja, VK_HANJA},
	{Keycode::escape, VK_ESCAPE},
	{Keycode::space, VK_SPACE},
	{Keycode::pageup, VK_PRIOR},
	{Keycode::pagedown, VK_NEXT},
	{Keycode::end, VK_END},
	{Keycode::home, VK_HOME},
	{Keycode::left, VK_LEFT},
	{Keycode::right, VK_RIGHT},
	{Keycode::up, VK_UP},
	{Keycode::down, VK_DOWN},
	{Keycode::select, VK_SELECT},
	{Keycode::print, VK_PRINT},
	{Keycode::insert, VK_INSERT},
	{Keycode::del, VK_DELETE},
	{Keycode::help, VK_HELP},
	{Keycode::leftmeta, VK_LWIN},
	{Keycode::rightmeta, VK_RWIN},
	{Keycode::sleep, VK_SLEEP},
	{Keycode::kp0, VK_NUMPAD0},
	{Keycode::kp1, VK_NUMPAD1},
	{Keycode::kp2, VK_NUMPAD2},
	{Keycode::kp3, VK_NUMPAD3},
	{Keycode::kp4, VK_NUMPAD4},
	{Keycode::kp5, VK_NUMPAD5},
	{Keycode::kp6, VK_NUMPAD6},
	{Keycode::kp7, VK_NUMPAD7},
	{Keycode::kp8, VK_NUMPAD8},
	{Keycode::kp9, VK_NUMPAD9},
	{Keycode::kpmultiply, VK_MULTIPLY},
	{Keycode::kpplus, VK_ADD},
	{Keycode::kpminus, VK_SUBTRACT},
	{Keycode::kpdivide, VK_DIVIDE},
	{Keycode::kpperiod, VK_SEPARATOR}, //XXX not sure
	{Keycode::f1, VK_F1},
	{Keycode::f2, VK_F2},
	{Keycode::f3, VK_F3},
	{Keycode::f4, VK_F4},
	{Keycode::f5, VK_F5},
	{Keycode::f6, VK_F6},
	{Keycode::f7, VK_F7},
	{Keycode::f8, VK_F8},
	{Keycode::f9, VK_F9},
	{Keycode::f10, VK_F10},
	{Keycode::f11, VK_F11},
	{Keycode::f12, VK_F12},
	{Keycode::f13, VK_F13},
	{Keycode::f14, VK_F14},
	{Keycode::f15, VK_F15},
	{Keycode::f16, VK_F16},
	{Keycode::f17, VK_F17},
	{Keycode::f18, VK_F18},
	{Keycode::f19, VK_F19},
	{Keycode::f20, VK_F20},
	{Keycode::f21, VK_F21},
	{Keycode::f22, VK_F22},
	{Keycode::f23, VK_F23},
	{Keycode::f24, VK_F24},
	{Keycode::numlock, VK_NUMLOCK},
	{Keycode::scrollock, VK_SCROLL},
	{Keycode::leftshift, VK_LSHIFT},
	{Keycode::rightshift, VK_RSHIFT},
	{Keycode::leftctrl, VK_LCONTROL},
	{Keycode::rightctrl, VK_RCONTROL},
	{Keycode::leftalt, VK_LMENU},
	{Keycode::rightalt, VK_RMENU},
	// XXX: some browser keys after this. not sure about it
	{Keycode::mute, VK_VOLUME_MUTE},
	{Keycode::volumedown, VK_VOLUME_DOWN},
	{Keycode::volumeup, VK_VOLUME_UP},
	{Keycode::nextsong, VK_MEDIA_NEXT_TRACK},
	{Keycode::previoussong, VK_MEDIA_PREV_TRACK},
	{Keycode::stopcd, VK_MEDIA_STOP}, // XXX: or keycode::stop?
	{Keycode::playpause, VK_MEDIA_PLAY_PAUSE},
	{Keycode::mail, VK_LAUNCH_MAIL},

	{Keycode::semicolon, VK_OEM_1},
	{Keycode::slash, VK_OEM_2},
	{Keycode::grave, VK_OEM_3},
	{Keycode::leftbrace, VK_OEM_4},
	{Keycode::backslash, VK_OEM_5},
	{Keycode::rightbrace, VK_OEM_6},
	{Keycode::apostrophe, VK_OEM_7},

	// XXX: something about OEM_102. is the 102nd keycode for linux/input.h

	{Keycode::play, VK_PLAY},
	{Keycode::zoom, VK_ZOOM},
};

constexpr struct EdgeConversion
{
	WindowEdge windowEdge;
	unsigned int winapiCode;
} edgeConversions[] = {
	{WindowEdge::top, 3u},
	{WindowEdge::bottom, 6u},
	{WindowEdge::left, 1u},
	{WindowEdge::right, 2u},
	{WindowEdge::topLeft, 4u},
	{WindowEdge::bottomLeft, 7u},
	{WindowEdge::topRight, 5u},
	{WindowEdge::bottomRight, 8u},
};

constexpr struct CursorConversion {
	CursorType cursor;
	const wchar_t* idc;
} cursorConversions[] = {
	{CursorType::leftPtr, IDC_ARROW},
	{CursorType::rightPtr, IDC_ARROW},
	{CursorType::load, IDC_WAIT},
	{CursorType::loadPtr, IDC_APPSTARTING},
	{CursorType::hand, IDC_HAND},
	{CursorType::grab, IDC_HAND},
	{CursorType::crosshair, IDC_CROSS},
	{CursorType::help, IDC_HELP},
	{CursorType::size, IDC_SIZEALL},
	{CursorType::sizeLeft, IDC_SIZEWE},
	{CursorType::sizeRight, IDC_SIZEWE},
	{CursorType::sizeTop, IDC_SIZENS},
	{CursorType::sizeBottom, IDC_SIZENS},
	{CursorType::sizeTopLeft, IDC_SIZENWSE},
	{CursorType::sizeBottomRight, IDC_SIZENWSE},
	{CursorType::sizeTopRight, IDC_SIZENESW},
	{CursorType::sizeBottomLeft, IDC_SIZENESW},
};

Keycode winapiToKeycode(unsigned int code)
{
	for(auto& kc : keycodeConversions)
		if(kc.vkcode == code) return kc.keycode;

	return Keycode::unkown;
}

unsigned int keycodeToWinapi(Keycode keycode)
{
	for(auto& kc : keycodeConversions)
		if(kc.keycode == keycode) return kc.vkcode;

	return 0u;
}

MouseButton winapiToButton(unsigned int code)
{
	switch(code)
	{
		case VK_RBUTTON: return MouseButton::right;
		case VK_LBUTTON: return MouseButton::left;
		case VK_MBUTTON: return MouseButton::middle;
		case VK_XBUTTON1: return MouseButton::custom1;
		case VK_XBUTTON2: return MouseButton::custom2;
		default: return MouseButton::none;
	}
}

unsigned int buttonToWinapi(MouseButton button)
{
	switch(button)
	{
		case MouseButton::right: return VK_RBUTTON;
		case MouseButton::left: return VK_LBUTTON;
		case MouseButton::middle: return VK_MBUTTON;
		case MouseButton::custom1: return VK_XBUTTON1;
		case MouseButton::custom2: return VK_XBUTTON2;
		default: return 0u;
	}
}

const wchar_t* cursorToWinapi(CursorType type)
{
	for(auto& cc : cursorConversions)
		if(cc.cursor == type) return cc.idc;

	return nullptr;
}

CursorType winapiToCursor(const wchar_t* idc)
{
	for(auto& cc : cursorConversions)
		if(cc.idc == idc) return cc.cursor;

	return CursorType::none;
}

unsigned int edgesToWinapi(WindowEdges edges)
{
	auto edge = static_cast<WindowEdge>(edges.value());
	for(auto& ec : edgeConversions)
		if(ec.windowEdge == edge) return ec.winapiCode;

	return 0u;
}

WindowEdge winapiToEdges(unsigned int edges)
{
	for(auto& ec : edgeConversions)
		if(ec.winapiCode == edges) return ec.windowEdge;

	return WindowEdge::none;
}

WinapiErrorCategory& WinapiErrorCategory::instance()
{
	static WinapiErrorCategory ret;
	return ret;
}

std::system_error WinapiErrorCategory::exception(std::string_view msg)
{
	if(msg.empty()) msg = "ny::Winapi: an error without message occurred";
	std::string msgn {msg};
	return std::system_error(std::error_code(::GetLastError(), instance()), msgn);
}

std::string WinapiErrorCategory::message(int code) const
{
	return winapi::errorMessage(code);
}

// The ugly hack our all lives depend on. You have found it. Congratulations!
// Srsly, this is done this way because we rely on windows treating wchar_t strings
// as utf16 strings. Strangely u16string is not the same as wstring on windows (mingw).
std::wstring widen(const std::string& string)
{
	auto str16 = nytl::toUtf16(string);
	return reinterpret_cast<const wchar_t*>(str16.c_str());
}

std::string narrow(const std::wstring& string)
{
	auto str16 = reinterpret_cast<const char16_t*>(string.c_str());
	return nytl::toUtf8(str16);
}

namespace winapi {

std::error_code lastErrorCode()
{
	return {static_cast<int>(::GetLastError()), WinapiErrorCategory::instance()};
}

std::system_error lastErrorException(std::string_view msg)
{
	return WinapiErrorCategory::exception(msg);
}

HBITMAP toBitmap(const Image& img)
{
	auto copy = convertFormat(img, ImageFormat::argb8888);
	premultiply(copy);

	BITMAPV5HEADER header {};
	header.bV5Size = sizeof(header);
	header.bV5Width = copy.size[0];
	header.bV5Height = -copy.size[1];
	header.bV5SizeImage = copy.size[1] * copy.stride;
	header.bV5Planes = 1;
	header.bV5BitCount = 32;
	header.bV5Compression = BI_RGB;

	auto hdc = ::GetDC(nullptr);
	auto hdcGuard = nytl::ScopeGuard([&]{ ::ReleaseDC(nullptr, hdc); });

	void* bitmapData {};
	auto& info = reinterpret_cast<BITMAPINFO&>(header);
	auto bitmap = ::CreateDIBSection(hdc, &info, DIB_RGB_COLORS, &bitmapData, nullptr, 0);

	std::memcpy(bitmapData, data(copy), dataSize(copy));
	return bitmap;
}

UniqueImage toImage(HBITMAP hbitmap)
{
	auto hdc = ::GetDC(nullptr);
	auto hdcGuard = nytl::ScopeGuard([&]{ ::ReleaseDC(nullptr, hdc); });

	::BITMAPINFO bminfo {};
	bminfo.bmiHeader.biSize = sizeof(bminfo.bmiHeader);

	if(!::GetDIBits(hdc, hbitmap, 0, 0, nullptr, &bminfo, DIB_RGB_COLORS)) {
		ny_warn("GetDiBits:1 failed");
		return {};
	}

	unsigned int width = bminfo.bmiHeader.biWidth;
	unsigned int height = std::abs(bminfo.bmiHeader.biHeight);
	unsigned int stride = width * 4;

	auto buffer = std::make_unique<uint8_t[]>(height * width * 4);

	bminfo.bmiHeader.biBitCount = 32;
	bminfo.bmiHeader.biCompression = BI_RGB;
	bminfo.bmiHeader.biHeight = height;

	if(!::GetDIBits(hdc, hbitmap, 0, height, buffer.get(), &bminfo, DIB_RGB_COLORS)) {
		ny_warn("GetDiBits:2 failed");
		return {};
	}

	UniqueImage ret;
	ret.data = std::move(buffer);
	ret.size = {width, height};
	ret.format = ImageFormat::argb8888;
	ret.stride = stride;

	return ret;
}

std::string errorMessage(unsigned int code, std::string_view msg)
{
	std::string ret;
	if(!msg.empty()) ret += msg;

	wchar_t buffer[512] = {};
	auto size = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, code, 0, buffer,
		sizeof(buffer), nullptr);

	if(!msg.empty()) ret += ": ";

	if(size > 0) ret += narrow(buffer);
	else ret += "<unkown winapi error>";

	return ret;
}

std::string errorMessage(std::string_view msg)
{
	return errorMessage(::GetLastError(), msg);
}

} // namespace winapi
} // namespace ny
