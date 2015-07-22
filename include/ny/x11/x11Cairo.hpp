#pragma once

#include <ny/x11/x11Include.hpp>
#include <ny/x11/x11WindowContext.hpp>

#include <cairo/cairo.h>

namespace ny
{

class x11CairoContext
{
protected:
    void cairoSetSize(vec2ui size);

    cairo_surface_t* cairoSurface_;
    cairoDrawContext* drawContext_;

public:
    x11CairoContext(x11WindowContext& wc);
    virtual ~x11CairoContext();

    cairo_surface_t* getCairoSurface() const { return cairoSurface_; }
};

}
