#pragma once

#include <ny/wayland/waylandInclude.hpp>

#include <ny/util/nonCopyable.hpp>
#include <ny/util/vec.hpp>
#include <ny/cairo.hpp>

namespace ny
{

////
class waylandCairoDrawContext: public cairoDrawContext
{
protected:
    const waylandWindowContext& wc_;
    wayland::shmBuffer* buffer_;

    void cairoSetSize(vec2ui size);

public:
    waylandCairoDrawContext(const waylandWindowContext& wc);
    virtual ~waylandCairoDrawContext();

    wayland::shmBuffer& getShmBuffer() const { return *buffer_; }
};


}
