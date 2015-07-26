#pragma once

#ifdef NY_WithCairo
#include <ny/include.hpp>
#include <ny/drawContext.hpp>

#include <cairo/cairo.h>

namespace ny
{

struct cairoFont
{
    cairo_font_face_t* handle;
};

class cairoDrawContext : public drawContext
{
protected:
    cairo_surface_t* cairoSurface_ = nullptr;
    cairo_t* cairoCR_ = nullptr;

    cairoDrawContext(surface& surf);

public:
    cairoDrawContext(surface& surf, cairo_surface_t& cairoSurface);
    virtual ~cairoDrawContext();

    virtual void clear(color col = color::none) override;

	virtual void mask(const customPath& obj) override;
	virtual void mask(const text& obj) override;
	virtual void resetMask() override;

	virtual void fill(const brush& col) override;
	virtual void outline(const pen& col) override;

    virtual rect2f getClip() override;
    virtual void clip(const rect2f& obj) override;
	virtual void resetClip() override;

	void save();
	void restore();

    cairo_surface_t* getCairoSurface() const { return cairoSurface_; };
    cairo_t* getCairoContext() const { return cairoCR_; };
};

}

#endif //Cairo

