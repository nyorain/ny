#pragma once

#include <ny/x11/x11Include.hpp>
#include <ny/cairo.hpp>

namespace ny
{

class x11CairoDrawContext : public cairoDrawContext
{
protected:
    void setSize(vec2ui size);

public:
    x11CairoDrawContext(x11WindowContext& wc);
    virtual ~x11CairoDrawContext();
};

}
