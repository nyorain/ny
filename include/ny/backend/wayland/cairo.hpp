#pragma once

#include <ny/backend/wayland/include.hpp>

#include <nytl/nonCopyable.hpp>
#include <nytl/vec.hpp>
#include <evg/cairo.hpp>

namespace ny
{

class WaylandCairoDrawContext: public evg::CairoDrawContext
{
public:
    WaylandCairoDrawContext(const WaylandWindowContext& wc);
    ~WaylandCairoDrawContext();

    void attach(const Vec2i& pos = Vec2i());
    void updateSize(const Vec2ui& size);

    void swapBuffers();
    bool frontBufferUsed() const;

protected:
    wayland::ShmBuffer* buffer_[2] {nullptr, nullptr};
    unsigned int frontID_ {0};

    cairo_surface_t* cairoBackSurface_ {nullptr};
    cairo_t* cairoBackCR_ {nullptr};

    wayland::ShmBuffer* frontBuffer() const { return buffer_[frontID_]; }
    wayland::ShmBuffer* backBuffer() const { return buffer_[frontID_^1]; }
};


}
