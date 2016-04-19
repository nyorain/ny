#pragma once

#include <ny/include.hpp>
#include <ny/draw/drawContext.hpp>
#include <nytl/cache.hpp>
#include <nytl/clone.hpp>

#include <windows.h>
#include <gdiplus.h>
using namespace Gdiplus;

namespace ny
{

///Font Handle for gdi fonts.
///Cache Name: "ny::GdiFontHandle"
class GdiFontHandle : public DeriveCloneable<Cache, GdiFontHandle>
{

};

///Gdi+ implementation of the DrawContext interface.
class GdiDrawContext : public DrawContext
{
protected:
    Graphics* graphics_ = nullptr;
    GraphicsPath currentPath_;

public:
	GdiDrawContext(Graphics& graphics);
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
};

}
