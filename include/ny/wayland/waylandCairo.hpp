#pragma once

#include <ny/wayland/waylandInclude.hpp>

#include <ny/util/nonCopyable.hpp>
#include <ny/util/vec.hpp>

#include <cairo/cairo.h>

namespace ny
{

////
class waylandCairoContext: public nonCopyable
{
protected:
    const waylandWindowContext& wc_;
    cairoDrawContext* drawContext_;

    wayland::shmBuffer* buffer_;
    cairo_surface_t* cairoSurface_;

    void cairoSetSize(vec2ui size);

public:
    waylandCairoContext(const waylandWindowContext& wc);
    virtual ~waylandCairoContext();

    cairo_surface_t& getCairoSurface() const { return *cairoSurface_; }
    cairoDrawContext& getDC() const { return *drawContext_; }
    wayland::shmBuffer& getShmBuffer() const { return *buffer_; }
};


}
