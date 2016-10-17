#include <ny/backend/winapi/input.hpp>
#include <ny/backend/winapi/windows.hpp>
#include <ny/backend/winapi/util.hpp>
#include <ny/backend/winapi/windowContext.hpp>
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
	return over_;
}

bool WinapiKeyboardContext::pressed(Keycode key) const
{
	auto keyState = ::GetAsyncKeyState(keycodeToWinapi(key));
	return ((1 << 16) & keyState);
}

std::string WinapiKeyboardContext::utf8(Keycode keycode, bool currentState) const
{
	auto vkcode = keycodeToWinapi(keycode);

	uint8_t state[256] {0};
	if(currentState) ::GetKeyboardState(state);

	wchar_t utf16[64];
	auto bytes = ::ToUnicode(vkcode, MapVirtualKey(vkcode, MAPVK_VK_TO_VSC), state, utf16, 6, 0);
	if(bytes <= 0) return {};

	utf16[bytes] = '\0';
	return nytl::toUtf8(static_cast<const wchar_t*>(utf16));
}

WindowContext* WinapiKeyboardContext::focus() const
{
	return focus_;
}

}
