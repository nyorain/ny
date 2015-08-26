#pragma once

#ifdef NY_WithCairo
#include <ny/include.hpp>
#include <ny/drawContext.hpp>

#include <cairo/cairo.h>

namespace ny
{

class cairoFont
{
protected:
    cairo_font_face_t* handle_;

public:
    cairoFont(const std::string& name, bool fromFile = 0);
    ~cairoFont();

    cairo_font_face_t* getFontFace() const { return handle_; }
};

class cairoDrawContext : public drawContext
{
protected:
    cairo_surface_t* cairoSurface_ = nullptr;
    cairo_t* cairoCR_ = nullptr;

    cairoDrawContext(surface& surf);

    void applyTransform(const transformable2& obj, vec2f pos = vec2f());
    void resetTransform();

public:
    cairoDrawContext(surface& surf, cairo_surface_t& cairoSurface);
    cairoDrawContext(image& img);
    virtual ~cairoDrawContext();

    virtual void clear(color col = color::none) override;

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

    cairo_surface_t* getCairoSurface() const { return cairoSurface_; };
    cairo_t* getCairoContext() const { return cairoCR_; };
};

}

#endif //Cairo

