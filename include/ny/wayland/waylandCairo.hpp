#pragma once

#include <ny/wayland/waylandInclude.hpp>

#include <nyutil/nonCopyable.hpp>
#include <nyutil/vec.hpp>
#include <ny/cairo.hpp>

namespace ny
{

////
class waylandCairoDrawContext: public cairoDrawContext
{
protected:
    const waylandWindowContext& wc_;
    wayland::shmBuffer* buffer_;

public:
    waylandCairoDrawContext(const waylandWindowContext& wc);
    virtual ~waylandCairoDrawContext();

    wayland::shmBuffer& getShmBuffer() const { return *buffer_; }
    void setSize(vec2ui size);
};


}
