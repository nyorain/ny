#include <ny/backend/winapi/gdi.hpp>

namespace ny
{

///DrawContext
GdiWindowDrawContext::GdiWindowDrawContext(HWND window) : window_(window)
{
}

void GdiWindowDrawContext::init()
{
	RECT rect;
	::GetClientRect(window_, &rect);
	auto size = Vec2ui(rect.right, rect.bottom);

	windowHdc_ = ::GetDC(window_);

	buffer_.reset(::CreateCompatibleBitmap(windowHdc_, rect.right, rect.bottom));

	hdc_ = ::CreateCompatibleDC(windowHdc_);
	oldBitmap_ = (HBITMAP) ::SelectObject(hdc(), buffer_.get());
	::SetGraphicsMode(hdc(), GM_ADVANCED);
}

void GdiWindowDrawContext::apply()
{
	resetTransform();

	RECT rect;
	::GetClientRect(window_, &rect);
	auto size = Vec2ui(rect.right, rect.bottom);

	::BitBlt(windowHdc_, 0, 0, size.x, size.y, hdc_, 0, 0, SRCCOPY);
	::ReleaseDC(window_, windowHdc_);

	SelectObject(hdc_, oldBitmap_);
	::DeleteDC(hdc_);
}

///WindowContext
GdiWinapiWindowContext::GdiWinapiWindowContext(WinapiAppContext& ctx,
	const WinapiWindowSettings& settings) : WinapiWindowContext(ctx, settings)
{
	drawContext_.reset(new GdiWindowDrawContext(handle()));
}

DrawGuard GdiWinapiWindowContext::draw()
{
	return DrawGuard(*drawContext_);
}

}
