#pragma once

#include <ny/include.hpp>
#include <ny/window.hpp>

namespace ny
{

class ChildWindow : public Window
{
protected:
	Window* parent_ = nullptr;

    ChildWindow();
    void create(Window& parent, const vec2ui& size, const WindowSettings& settings = {});

public:
    ChildWindow(Window& parent, const vec2ui& size, const WindowSettings& settings = {});

    //const ToplevelWindow* topLevelParent() const { return parent()->getTopLevelParent(); };
    //ToplevelWindow* topLevelParent() { return parent()->getTopLevelParent(); };
	
	Window& parent() const { return *parent_; }
};

}
