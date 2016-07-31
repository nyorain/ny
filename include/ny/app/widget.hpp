#pragma once

#include <ny/include.hpp>
#include <ny/window.hpp>

namespace ny
{

class widget : public childWindow
{
public:
    using childWindow::childWindow;

    virtual ~widget() = default;

    virtual std::string getWidgetName() const = 0;
};

}
