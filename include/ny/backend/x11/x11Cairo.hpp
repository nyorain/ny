#pragma once

#include <ny/x11/x11Include.hpp>
#include <ny/cairo.hpp>

namespace ny
{

class x11CairoDrawContext : public cairoDrawContext
{
protected:
    cairo_surface_t* xlibSurface_ = nullptr; //to make it double buffered

public:
    x11CairoDrawContext(x11WindowContext& wc);
    virtual ~x11CairoDrawContext();

    void setSize(vec2ui size);
    virtual void apply() override;
};

}
