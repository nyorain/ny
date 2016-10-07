#include <ny/backend/winapi/input.hpp>
#include <ny/backend/winapi/windows.hpp>
#include <ny/backend/winapi/util.hpp>
#include <nytl/utf.hpp>

namespace ny
{

Vec2ui WinapiMouseContext::position() const
{
	POINT p;
	if(!::GetCursorPos(&p)) return {};

	HWND hwnd;
	if(!::ScreenToClient(hwnd, &p)) return {};

	return Vec2i(p.x, p.y);
}
bool WinapiMouseContext::pressed(MouseButton button) const
{
	auto keyState = ::GetAsyncKeyState(buttonToWinapi(button));
	return ((1 << 16) & keyState);
}
WindowContext* WinapiMouseContext::over() const
{

}

bool WinapiKeyboardContext::pressed(Key key) const
{
	auto keyState = ::GetAsyncKeyState(keyToWinapi(key));
	return ((1 << 16) & keyState);
}
std::string WinapiKeyboardContext::unicode(Key key) const
{
	return unicode(keyToWinapi(key));
}
std::string WinapiKeyboardContext::unicode(unsigned int vkcode) const
{
	std::uint8_t kb[256];
	::GetKeyboardState(kb);
	wchar_t unicode[6];
	::ToUnicode(vkcode, MapVirtualKey(vkcode, MAPVK_VK_TO_VSC), kb, unicode, 6, 0);
	return nytl::toUtf8(unicode);
}
WindowContext* WinapiKeyboardContext::focus() const
{

}

}
