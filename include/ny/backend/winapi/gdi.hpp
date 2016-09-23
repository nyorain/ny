#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <evg/gdi.hpp>

#include <memory>

namespace ny
{

///GdiDrawContext to draw on a winapi window
class GdiWindowDrawContext : public evg::GdiDrawContext
{
protected:
	HWND window_ {nullptr};

	HBITMAP oldBitmap_;
	GdiPointer<HBITMAP> buffer_;
	HDC windowHdc_;

public:
	GdiWindowDrawContext(HWND window);

	void init() override;
	void apply() override;
};

///Winapi WindowContext using gdi to draw.
class GdiWinapiWindowContext : public WinapiWindowContext
{
protected:
	std::unique_ptr<GdiWindowDrawContext> drawContext_;

public:
	GdiWinapiWindowContext(WinapiAppContext& ctx, const WinapiWindowSettings& settings = {});
};

}
