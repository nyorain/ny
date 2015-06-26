#pragma once

#include <ny/include.hpp>
#include <ny/window/window.hpp>

namespace ny
{

class frame : public toplevelWindow
{
public:
    frame(vec2i position, vec2ui size);
    virtual ~frame();
};

}
