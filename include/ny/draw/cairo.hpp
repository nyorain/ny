#pragma once

#include <ny/include.hpp>
#include <ny/draw/drawContext.hpp>

//prototypes
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;

namespace ny
{

///The cairo implementation for the DrawContext interface.
class CairoDrawContext : public DrawContext
{
protected:
    cairo_surface_t* cairoSurface_ = nullptr; 
    cairo_t* cairoCR_ = nullptr;

protected:
    CairoDrawContext();

    void applyTransform(const transform2& xtransform);
    void resetTransform();

public:
    CairoDrawContext(cairo_surface_t& cairoSurface);
    CairoDrawContext(Image& img);
    virtual ~CairoDrawContext();

    bool init(cairo_surface_t& cairoSurface);

    virtual void clear(const Brush& b = Brush::none) override;
	virtual void paint(const Brush& b, const Brush& alpha) override;
    virtual void apply() override;

	virtual void mask(const Path& obj) override;
	virtual void mask(const Text& obj) override;
	virtual void mask(const Rectangle& obj) override;
	virtual void mask(const Circle& obj) override;
	virtual void resetMask() override;

	virtual void fill(const Brush& col) override;
	virtual void stroke(const Pen& col) override;

    virtual void fillPreserve(const Brush& col) override;
	virtual void strokePreserve(const Pen& col) override;

    virtual Rect2f RectangleClip() const override;
    virtual void clipRectangle(const Rect2f& obj) override;
	virtual void resetRectangleClip() override;

	//
	void saveCairo();
	void restoreCairo();

    cairo_surface_t* cairoSurface() const { return cairoSurface_; };
    cairo_t* cairoContext() const { return cairoCR_; };
};

}

