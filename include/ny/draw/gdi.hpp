#pragma once

#include <ny/include.hpp>
#include <ny/draw/drawContext.hpp>
#include <nytl/cache.hpp>
#include <nytl/clone.hpp>

#include <windows.h>
#include <gdiplus.h>

namespace ny
{

///FontFamily Handle for gdi fonts.
///Cache Name: "ny::GdiFontHandle"
class GdiFontHandle : public DeriveCloneable<Cache, GdiFontHandle>
{
protected:
	std::unique_ptr<Gdiplus::FontFamily> handle_;

public:
	GdiFontHandle(const Font& font);
	GdiFontHandle(const std::string& name, bool fromFile);
	~GdiFontHandle() { std::cout << "blrr" << handle_.get() << "\n"; };

	GdiFontHandle(const GdiFontHandle& other);
	GdiFontHandle& operator=(const GdiFontHandle& other);

	GdiFontHandle(GdiFontHandle&& other) noexcept = default;
	GdiFontHandle& operator=(GdiFontHandle&& other) noexcept = default;

	const Gdiplus::FontFamily& handle() const { return *handle_; }
	Gdiplus::FontFamily& handle() { return *handle_; }
};

///TODO: gdiplus graphics object moveable? constructor with move
///Gdi+ implementation of the DrawContext interface.
class GdiDrawContext : public DelayedDrawContext
{
public:
	GdiDrawContext(HDC hdc);
	GdiDrawContext(HDC hdc, HANDLE handle);
	GdiDrawContext(Gdiplus::Image& gdiimage);
	GdiDrawContext(HWND window, bool adjust = true);
	virtual ~GdiDrawContext();

	virtual void clear(const Brush& b = Brush::none) override;
	virtual void paint(const Brush& alphaMask, const Brush& brush) override;

	virtual void fillPreserve(const Brush& brush) override;
	virtual void strokePreserve(const Pen& pen) override;

	//TODO: draw shape functions, more efficient directly

    virtual Rect2f rectangleClip() const override;
    virtual void clipRectangle(const Rect2f& obj) override;
	virtual void resetRectangleClip() override;

	//specific
	Gdiplus::Graphics& graphics() { return graphics_; }
	const Gdiplus::Graphics& graphics() const { return graphics_; }

	void setTransform(const Transform2& transform);
	void setTransform(const Mat3f& m);

protected:
	void gdiFill(const Path& obj, const Gdiplus::Brush& brush);
	void gdiFill(const Text& obj, const Gdiplus::Brush& brush);
	void gdiFill(const Rectangle& obj, const Gdiplus::Brush& brush);
	void gdiFill(const Circle& obj, const Gdiplus::Brush& brush);

	void gdiStroke(const Path& obj, const Gdiplus::Pen& pen);
	void gdiStroke(const Text& obj, const Gdiplus::Pen& pen);
	void gdiStroke(const Rectangle& obj, const Gdiplus::Pen& pen);
	void gdiStroke(const Circle& obj, const Gdiplus::Pen& pen);

protected:
    Gdiplus::Graphics graphics_;
};

}
