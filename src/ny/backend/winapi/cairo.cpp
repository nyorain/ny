#include <ny/backend/winapi/cairo.hpp>
#include <ny/backend/winapi/appContext.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/base/log.hpp>

#include <cairo/cairo-win32.h>

namespace ny
{

//backend/integration/cairo.cpp - private function
using CairoIntegrateFunc = std::function<std::unique_ptr<CairoIntegration>(WindowContext& context)>;
unsigned int registerCairoIntegrateFunc(const CairoIntegrateFunc& func);

namespace
{
	std::unique_ptr<CairoIntegration> winapiCairoIntegrateFunc(WindowContext& windowContext)
	{
		auto* xwc = dynamic_cast<WinapiWindowContext*>(&windowContext);
		if(!xwc) return nullptr;

		try { auto ret = std::make_unique<WinapiCairoIntegration>(*xwc); return ret; }
		catch(const std::exception&) {}

		return nullptr;
	}

	static int registered = registerCairoIntegrateFunc(winapiCairoIntegrateFunc);
}

WinapiCairoIntegration::WinapiCairoIntegration(WinapiWindowContext& wc)
	: WinapiDrawIntegration(wc)
{
}

WinapiCairoIntegration::~WinapiCairoIntegration()
{
}

HBITMAP orig;
HBITMAP bitmap;

cairo_surface_t& WinapiCairoIntegration::init()
{

	/*
	auto extents = windowContext_.clientExtents();
	auto surface = cairo_win32_surface_create_with_dib(CAIRO_FORMAT_ARGB32, extents.width(), extents.height());
	return *surface;
	*/


	// auto hdc = ::GetDC(windowContext_.handle());
	auto extents = windowContext_.extents();

	HDC hdcScreen = GetDC(NULL);
	// HDC hdc = CreateCompatibleDC(hdcScreen);

	// bitmap = ::CreateBitmap(extents.width(), extents.height(), 1, 32, nullptr);
	// bitmap = ::CreateCompatibleBitmap(hdcScreen, extents.width(), extents.height());
	// orig = (HBITMAP) SelectObject(hdc, bitmap);

	// auto surface = cairo_win32_surface_create(hdc);

	// debug("e: ", extents);
	//
	auto surface = cairo_win32_surface_create_with_ddb(hdcScreen, CAIRO_FORMAT_ARGB32, extents.width(), extents.height());
	::ReleaseDC(nullptr, hdcScreen);

	return *surface;
}
void WinapiCairoIntegration::apply(cairo_surface_t& surface)
{
	HDC hdcScreen = GetDC(NULL);

	cairo_surface_flush(&surface);
	auto hdc = cairo_win32_surface_get_dc(&surface);

	debug("h: ", hdc);

	BLENDFUNCTION blend;
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.SourceConstantAlpha = 255;
	blend.AlphaFormat = AC_SRC_ALPHA;
	SIZE sizeWnd = {windowContext_.extents().width(), windowContext_.extents().height()};
	POINT ptSrc = {0, 0};

	SetWindowPos(windowContext_.handle(), 0, 0, 0, 0, 0,
		SWP_DRAWFRAME|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER);

	UpdateLayeredWindow(windowContext_.handle(), hdcScreen, nullptr, &sizeWnd, hdc, &ptSrc, 0, &blend, ULW_ALPHA);
	// ::ShowWindow(windowContext_.handle(), SW_SHOWDEFAULT);
	// ::UpdateWindow(windowContext_.handle());
	//
	//
	// ::DefWindowProc(windowContext_.handle(), WM_NCHITTEST, 0, 0);
	//
	// auto region = ::CreateRectRgn(0, 0, windowContext_.extents().width(), windowContext_.extents().height());
	// ::DefWindowProc(windowContext_.handle(), WM_NCPAINT, (long long) region, 0);
	// ::DeleteObject(region);

	SetWindowPos(windowContext_.handle(), 0, 0, 0, 0, 0,
		SWP_DRAWFRAME|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER);

	::ReleaseDC(nullptr, hdcScreen);
	cairo_surface_destroy(&surface);


	//alternative
	/*
	cairo_surface_flush(&surface);
	auto hdc = cairo_win32_surface_get_dc(&surface);

	auto extents = windowContext_.clientExtents();
	auto whdc = ::GetDC(windowContext_.handle());
	BitBlt(whdc, 0, 0, extents.width(), extents.height(), hdc, 0, 0, SRCCOPY);
	ReleaseDC(windowContext_.handle(), hdc);
	cairo_surface_destroy(&surface);
	*/
}

// RECT rect {0, 0, 500, 100};
// ::DrawFrameControl(hdc, &rect, DFC_CAPTION, DFCS_CAPTIONCLOSE);
// ::ReleaseDC(windowContext_.handle(), hdc);
// ::SelectObject(hdc, orig);
// ::DeleteDC(hdc);
// ::DeleteObject(bitmap);

}
