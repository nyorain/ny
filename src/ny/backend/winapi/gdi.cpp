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
	GetClientRect(window_, &rect);

	//hdc_ = BeginPaint(window_, &ps_);
	hdc_ = GetDC(window_);

	//buffer_.reset(new Gdiplus::Bitmap(rect.right, rect.bottom));
	//graphics_.reset(new Gdiplus::Graphics(buffer_.get()));
	//graphics_.reset(new Gdiplus::Graphics(hdc_));

	//windowGraphics_.reset(new Gdiplus::Graphics(hdc_));
}

void GdiWindowDrawContext::apply()
{
	//windowGraphics_->DrawImage(buffer_.get(), 0, 0);
	ReleaseDC(window_, hdc_);
	//EndPaint(window_, &ps_);
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
