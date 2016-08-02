#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/draw/gdi.hpp>

#include <memory>

namespace ny
{

///GdiDrawContext to draw on a winapi window
class GdiWindowDrawContext : public GdiDrawContext
{
protected:
	HWND window_ {nullptr};

	HBITMAP oldBitmap_;
	GdiPointer<HBITMAP> buffer_;
	HDC windowHdc_;

public:
	GdiWindowDrawContext(HWND window);

	virtual void init() override;
	virtual void apply() override;
};

///Winapi WindowContext using gdi to draw.
class GdiWinapiWindowContext : public WinapiWindowContext
{
protected:
	std::unique_ptr<GdiWindowDrawContext> drawContext_;

public:
	GdiWinapiWindowContext(WinapiAppContext& ctx, const WinapiWindowSettings& settings = {});
	virtual DrawGuard draw() override;
};

}
