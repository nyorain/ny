#pragma once

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
    cairo_surface_t* cairoSurface_;
    cairo_t* cairoCR_;

    cairoDrawContext(surface& surf);

public:
    cairoDrawContext(surface& surf, cairo_surface_t& cairoSurface);
    virtual ~cairoDrawContext();

    virtual void clear(color col = color::none);

	virtual void mask(const customPath& obj);
	virtual void mask(const text& obj);
	virtual void resetMask();

	virtual void fill(const brush& col);
	virtual void outline(const pen& col);

    virtual rect2f getClip();
    virtual void clip(const rect2f& obj);
	virtual void resetClip();

	void save();
	void restore();

    cairo_surface_t* getCairoSurface() const { return cairoSurface_; };
    cairo_t* getCairoContext() const { return cairoCR_; };
};

}

