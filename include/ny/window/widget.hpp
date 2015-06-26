#pragma once

#include <ny/include.hpp>
#include <ny/window/window.hpp>

namespace ny
{

class widget : public childWindow
{
public:
    widget(window* parent, vec2i position, vec2ui size);
    virtual ~widget();
};

}
