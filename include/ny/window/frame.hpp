#pragma once

#include "include.hpp"
#include "window.hpp"

namespace ny
{

class frame : public toplevelWindow
{
public:
    frame(vec2i position, vec2ui size);
    virtual ~frame();
};

}
