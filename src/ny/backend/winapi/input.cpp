#include <ny/backend/winapi/input.hpp>
#include <ny/backend/winapi/windows.hpp>
#include <ny/backend/winapi/util.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <nytl/utf.hpp>

namespace ny
{

Vec2ui WinapiMouseContext::position() const
{
	if(!over_) return {};

	POINT p;
	if(!::GetCursorPos(&p)) return {};
	if(!::ScreenToClient(over_->handle(), &p)) return {};

	return Vec2i(p.x, p.y);
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
	auto bytes = ::ToUnicode(vkcode, MapVirtualKey(vkcode, MAPVK_VK_TO_VSC), state, utf16, 64, 0);
	if(bytes <= 0) return {};

	utf16[bytes] = '\0';
	return nytl::toUtf8(static_cast<const wchar_t*>(utf16));
}

void WinapiKeyboardContext::focus(WinapiWindowContext* wc)
{
	if(wc == focus_) return;

	onFocus(*this, focus_, wc);
	focus_ = wc;
}

}
