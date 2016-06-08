#include <ny/backend/winapi/util.hpp>
#include <windows.h>

namespace ny
{

Keyboard::Key winapiToKey(unsigned int code)
{
	if(code >= 0x30 && code <= 0x39) return static_cast<Keyboard::Key>(27 + code - 0x30);
	if(code >= 0x41 && code <= 0x5A) return static_cast<Keyboard::Key>(code - 0x41);
	if(code >= 0x60 && code <= 0x69) return static_cast<Keyboard::Key>(37 + code - 0x60);
	if(code >= 0x70 && code <= 0x87) return static_cast<Keyboard::Key>(47 + code - 0x70);

	switch(code)
	{
		case VK_BACK: return Keyboard::Key::back;
		case VK_TAB: return Keyboard::Key::tab;
		case VK_RETURN: return Keyboard::Key::enter;
		case VK_SHIFT: return Keyboard::Key::leftshift;
		case VK_CONTROL: return Keyboard::Key::leftctrl;
		case VK_MENU: return Keyboard::Key::leftalt;
		case VK_PAUSE: return Keyboard::Key::pause;
		case VK_CAPITAL: return Keyboard::Key::capsLock;
		case VK_ESCAPE: return Keyboard::Key::escape;
		case VK_SPACE: return Keyboard::Key::space;
		case VK_PRIOR: return Keyboard::Key::pageUp;
		case VK_NEXT: return Keyboard::Key::pageDown;
		case VK_END: return Keyboard::Key::end;
		case VK_HOME: return Keyboard::Key::home;
		case VK_LEFT: return Keyboard::Key::left;
		case VK_RIGHT: return Keyboard::Key::right;
		case VK_UP: return Keyboard::Key::up;
		case VK_DOWN: return Keyboard::Key::down;
		case VK_DELETE: return Keyboard::Key::del;
		case VK_INSERT: return Keyboard::Key::insert;
		case VK_LWIN: return Keyboard::Key::leftsuper;
		case VK_RWIN: return Keyboard::Key::rightsuper;
		default: return Keyboard::Key::none;
	}
}

unsigned int keyToWinapi(Keyboard::Key key)
{
	auto code = static_cast<unsigned int>(key);

	//TODO
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

}
