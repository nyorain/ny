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
    void create(Window& parent, const vec2ui& size, const WindowContextSettings& settings = {});

public:
    ChildWindow(Window& parent, const vec2ui& size, const WindowContextSettings& settings = {});

    //const ToplevelWindow* topLevelParent() const { return getParent()->getTopLevelParent(); };
    //ToplevelWindow* topLevelParent() { return getParent()->getTopLevelParent(); };
	
	Window& parent() const { return *parent_; }
};

}
