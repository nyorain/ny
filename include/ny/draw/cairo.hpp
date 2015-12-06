#pragma once

#include <ny/draw/include.hpp>

#ifdef NY_WithCairo
#include <ny/drawContext.hpp>

#include <cairo/cairo.h>

namespace ny
{

///The cairo implementation for the DrawContext interface.
class CairoDrawContext : public DrawContext
{
protected:
    cairo_surface_t* cairoSurface_ = nullptr; //both prob. better custom unique_ptr, to dont have to care about onwership (destruction) with derived classes
    cairo_t* cairoCR_ = nullptr;

    cairoDrawContext();

    void applyTransform(const transformable2& obj);
    void resetTransform();

public:
    cairoDrawContext(cairo_surface_t& cairoSurface);
    cairoDrawContext(image& img);
    virtual ~cairoDrawContext();

    bool init(cairo_surface_t& cairoSurface);

    virtual void clear(color col = color::none) override;
    virtual void apply() override;

	virtual void mask(const customPath& obj) override;
	virtual void mask(const text& obj) override;
	virtual void mask(const rectangle& obj) override;
	virtual void mask(const circle& obj) override;
	virtual void resetMask() override;

	virtual void fill(const brush& col) override;
	virtual void stroke(const pen& col) override;

    virtual void fillPreserve(const brush& col) override;
	virtual void strokePreserve(const pen& col) override;

    virtual rect2f getClip() override;
    virtual void clip(const rect2f& obj) override;
	virtual void resetClip() override;

	void save();
	void restore();

    cairo_surface_t* cairoSurface() const { return cairoSurface_; };
    cairo_t* cairoContext() const { return cairoCR_; };
};

}

#endif //Cairo
