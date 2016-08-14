#include <ny/backend/winapi/util.hpp>
#include <ny/backend/winapi/windows.hpp>
#include <ny/base/cursor.hpp>
#include <ny/backend/keyboardContext.hpp>
#include <ny/backend/mouseContext.hpp>

namespace ny
{

Key winapiToKey(unsigned int code)
{
	if(code >= 0x30 && code <= 0x39) return static_cast<Key>(27 + code - 0x30);
	if(code >= 0x41 && code <= 0x5A) return static_cast<Key>(code - 0x41);
	if(code >= 0x60 && code <= 0x69) return static_cast<Key>(37 + code - 0x60);
	if(code >= 0x70 && code <= 0x87) return static_cast<Key>(47 + code - 0x70);

	switch(code)
	{
		case VK_BACK: return Key::back;
		case VK_TAB: return Key::tab;
		case VK_RETURN: return Key::enter;
		case VK_SHIFT: return Key::leftshift;
		case VK_CONTROL: return Key::leftctrl;
		case VK_MENU: return Key::leftalt;
		case VK_PAUSE: return Key::pause;
		case VK_CAPITAL: return Key::capsLock;
		case VK_ESCAPE: return Key::escape;
		case VK_SPACE: return Key::space;
		case VK_PRIOR: return Key::pageUp;
		case VK_NEXT: return Key::pageDown;
		case VK_END: return Key::end;
		case VK_HOME: return Key::home;
		case VK_LEFT: return Key::left;
		case VK_RIGHT: return Key::right;
		case VK_UP: return Key::up;
		case VK_DOWN: return Key::down;
		case VK_DELETE: return Key::del;
		case VK_INSERT: return Key::insert;
		case VK_LWIN: return Key::leftsuper;
		case VK_RWIN: return Key::rightsuper;
		default: return Key::none;
	}
}

unsigned int keyToWinapi(Key key)
{
	auto code = static_cast<unsigned int>(key);

	if(code >= 27 && code <= 36) return 0x30 + code - 27;
	if(code >= 1 && code <= 26) return code + 0x41;
	if(code >= 37 && code <= 46) return 0x60 + code - 37;
	if(code >= 47 && code <= 70) return 0x70 + code - 47;

	switch(key)
	{
		case Key::back: return VK_BACK;
		case Key::tab: return VK_TAB;
		case Key::enter: return VK_RETURN;
		case Key::leftshift: return VK_SHIFT;
		case Key::leftctrl: return VK_CONTROL;
		case Key::leftalt: return VK_MENU;
		case Key::pause: return VK_PAUSE;
		case Key::capsLock: return VK_CAPITAL;
		case Key::escape: return VK_ESCAPE;
		case Key::space: return VK_SPACE;
		case Key::pageUp: return VK_PRIOR;
		case Key::pageDown: return VK_NEXT;
		case Key::end: return VK_END;
		case Key::home: return VK_HOME;
		case Key::left: return VK_LEFT;
		case Key::right: return VK_RIGHT;
		case Key::up: return VK_UP;
		case Key::down: return VK_DOWN;
		case Key::del: return VK_DELETE;
		case Key::insert: return VK_INSERT;
		case Key::leftsuper: return VK_LWIN;
		case Key::rightsuper: return VK_RWIN;
		default: return 0;
	}
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

	if(size > 0 && msg) ret += ": ";
	ret += buffer;

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

}
