#pragma once

#include <ny/include.hpp>
#include <ny/draw/drawContext.hpp>
#include <nytl/cache.hpp>
#include <nytl/clone.hpp>

#include <windows.h>
#include <gdiplus.h>

namespace ny
{

///Font Handle for gdi fonts.
///Cache Name: "ny::GdiFontHandle"
class GdiFontHandle : public DeriveCloneable<Cache, GdiFontHandle>
{

};

///TODO: gdiplus graphics object moveable? constructor with move
///Gdi+ implementation of the DrawContext interface.
class GdiDrawContext : public DrawContext
{
protected:
    Gdiplus::Graphics graphics_;
    Gdiplus::GraphicsPath currentPath_;

public:
	GdiDrawContext(HDC hdc);
	GdiDrawContext(HDC hdc, HANDLE handle);
	GdiDrawContext(Gdiplus::Image& gdiimage);
	GdiDrawContext(HWND window, bool adjust = true);
	virtual ~GdiDrawContext();

	virtual void clear(const Brush& b = Brush::none) override;
	virtual void paint(const Brush& alphaMask, const Brush& brush) override;

	virtual void mask(const Path& obj) override;
	virtual void mask(const Text& obj) override;
	virtual void mask(const Rectangle& obj) override;
	virtual void mask(const Circle& obj) override;
	virtual void resetMask() override;

	virtual void fill(const Brush& brush) override;
	virtual void fillPreserve(const Brush& brush) override;
	virtual void stroke(const Pen& pen) override;
	virtual void strokePreserve(const Pen& pen) override;

    virtual Rect2f rectangleClip() const override;
    virtual void clipRectangle(const Rect2f& obj) override;
	virtual void resetRectangleClip() override;

	//specific
	Gdiplus::Graphics& graphics() { return graphics_; }
	const Gdiplus::Graphics& graphics() const { return graphics_; }

	Gdiplus::GraphicsPath& currentPath() { return currentPath_; }
	const Gdiplus::GraphicsPath& currentPath() const { return currentPath_; }
};

}
