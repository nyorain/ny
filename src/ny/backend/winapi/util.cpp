#include <ny/backend/winapi/util.hpp>
#include <ny/backend/winapi/windows.hpp>
#include <ny/backend/mouseContext.hpp>
#include <ny/base/cursor.hpp>
#include <ny/base/key.hpp>

namespace ny
{

//maps ny::Keycode to a windows-specific virtual key code.
constexpr struct Mapping
{
	Keycode keycode;
	unsigned int vkcode;
} mappings [] =
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
	//XXX: some browser keys after this. not sure
	{Keycode::mute, VK_VOLUME_MUTE},
	{Keycode::volumedown, VK_VOLUME_DOWN},
	{Keycode::volumeup, VK_VOLUME_UP},
	{Keycode::nextsong, VK_MEDIA_NEXT_TRACK},
	{Keycode::previoussong, VK_MEDIA_PREV_TRACK},
	{Keycode::stopcd, VK_MEDIA_STOP}, //XXX: or keycode::stop?
	{Keycode::playpause, VK_MEDIA_PLAY_PAUSE},
	{Keycode::mail, VK_LAUNCH_MAIL},

	{Keycode::semicolon, VK_OEM_1},
	{Keycode::slash, VK_OEM_2},
	{Keycode::grave, VK_OEM_3},
	{Keycode::leftbrace, VK_OEM_4},
	{Keycode::backslash, VK_OEM_5},
	{Keycode::rightbrace, VK_OEM_6},
	{Keycode::apostrophe, VK_OEM_7},

	//XXX: something about OEM_102. is the 102nd keycode for linux/input.h

	{Keycode::play, VK_PLAY},
	{Keycode::zoom, VK_ZOOM},
};

Keycode winapiToKeycode(unsigned int code)
{
	for(auto& m : mappings)
	{
		if(m.vkcode == code) return m.keycode;
	}

	return Keycode::unkown;
}

unsigned int keycodeToWinapi(Keycode keycode)
{
	for(auto& m : mappings)
	{
		if(m.keycode == keycode) return m.vkcode;
	}

	return 0x0;
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
		default: return 0;
	}
}

std::string errorMessage(unsigned int code, const char* msg)
{
	std::string ret;
	if(msg) ret += msg;

	char buffer[256] = {};
	auto size = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, code, 0, buffer,
		sizeof(buffer), nullptr);

	if(msg) ret += ": ";

	if(size > 0) ret += buffer;
	else ret += "<unkown winapi error>";

	return ret;
}

std::string errorMessage(const char* msg)
{
	return errorMessage(::GetLastError(), msg);
}

const char* cursorToWinapi(CursorType type)
{
	switch(type)
	{
		case CursorType::leftPtr: return IDC_ARROW;
		case CursorType::rightPtr: return IDC_ARROW;
		case CursorType::load: return IDC_WAIT;
		case CursorType::loadPtr: return IDC_APPSTARTING;
		case CursorType::hand: return IDC_HAND;
		case CursorType::grab: return IDC_HAND;
		case CursorType::crosshair: return IDC_CROSS;
		case CursorType::help: return IDC_HELP;
		case CursorType::size: return IDC_SIZEALL;
		case CursorType::sizeLeft: return IDC_SIZENS;
		case CursorType::sizeRight: return IDC_SIZEWE;
		case CursorType::sizeTop: return IDC_SIZENS;
		case CursorType::sizeBottom: return IDC_SIZEWE;
		case CursorType::sizeBottomRight: return IDC_SIZENWSE;
		case CursorType::sizeBottomLeft: return IDC_SIZENESW;
		case CursorType::sizeTopRight: return IDC_SIZENESW;
		case CursorType::sizeTopLeft: return IDC_SIZENWSE;
		// case Cursor::Type::no: return IDC_NO;
		default: return nullptr;
	}
}

WinapiErrorCategory& WinapiErrorCategory::instance()
{
	static WinapiErrorCategory ret;
	return ret;
}

std::string WinapiErrorCategory::message(int code) const
{
	return errorMessage(code);
}

}
