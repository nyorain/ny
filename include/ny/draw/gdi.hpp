#pragma once

#include <ny/include.hpp>
#include <ny/draw/drawContext.hpp>
#include <nytl/cache.hpp>
#include <nytl/clone.hpp>

#include <windows.h>

namespace ny
{

///Can be used on unique pointers for winapi handles.
struct GdiObjectDestructor
{
	void operator()(HGDIOBJ obj) const { DeleteObject(obj); }
};

template <typename T>
using GdiPointer = std::unique_ptr<std::remove_pointer_t<T>, GdiObjectDestructor>;

///FontFamily Handle for gdi fonts.
///Cache Name: "ny::GdiFontHandle"
class GdiFontHandle : public Cache
{
public:
	GdiFontHandle(const Font& font);
	GdiFontHandle(const std::string& name, bool fromFile);
	~GdiFontHandle() = default;

	GdiFontHandle(GdiFontHandle&& other) noexcept = default;
	GdiFontHandle& operator=(GdiFontHandle&& other) noexcept = default;

	HFONT handle() { return handle_.get(); }

protected:
	GdiPointer<HFONT> handle_;
};

///Gdi implementation of the DrawContext interface.
class GdiDrawContext : public DelayedDrawContext
{
public:
	GdiDrawContext(HDC hdc);
	virtual ~GdiDrawContext();

	virtual void clear(const Brush& b = Brush::none) override;
	virtual void paint(const Brush& alphaMask, const Brush& brush) override;

	virtual void fillPreserve(const Brush& brush) override;
	virtual void strokePreserve(const Pen& pen) override;

	//TODO: draw shape functions, more efficient directly
	// virtual void draw(const Shape& shape) override;

    virtual Rect2f rectangleClip() const override;
    virtual void clipRectangle(const Rect2f& obj) override;
	virtual void resetRectangleClip() override;

	//specific
	HDC hdc() const { return hdc_; }

	void setTransform(const Transform2& transform);
	void setTransform(const Mat3f& m);
	void resetTransform();

protected:
	GdiDrawContext() = default;

	void gdiFill(const Path& obj, const Brush& brush);
	void gdiFill(const Text& obj, const Brush& brush);
	void gdiFill(const Rectangle& obj, const Brush& brush);
	void gdiFill(const Circle& obj, const Brush& brush);

	void gdiStroke(const Path& obj, const Pen& pen);
	void gdiStroke(const Text& obj, const Pen& pen);
	void gdiStroke(const Rectangle& obj, const Pen& pen);
	void gdiStroke(const Circle& obj, const Pen& pen);

protected:
	HDC hdc_;
};

}
