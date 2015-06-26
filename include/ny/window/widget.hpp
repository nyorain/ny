#pragma once

#include "include.hpp"
#include "window.hpp"

namespace ny
{

class widget : public childWindow
{
public:
    widget(window* parent, vec2i position, vec2ui size);
    virtual ~widget();
};

}
