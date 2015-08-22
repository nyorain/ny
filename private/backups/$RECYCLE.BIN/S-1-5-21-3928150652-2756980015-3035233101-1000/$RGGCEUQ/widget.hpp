#pragma once

#include <ny/include.hpp>
#include <ny/window.hpp>

namespace ny
{

class widget : public childWindow
{
public:
    widget(window& parent, vec2i position, vec2ui size, const windowContextSettings& = windowContextSettings());
    virtual ~widget() = default;

    virtual std::string getWidgetName() const = 0;
};

}
