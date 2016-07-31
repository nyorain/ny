#pragma once

#include <ny/include.hpp>
#include <ny/window/window.hpp>

namespace ny
{

class ChildWindow : public Window
{
protected:
	Window* parent_ = nullptr;

    ChildWindow();
    void create(Window& parent, const Vec2ui& size, const WindowSettings& settings = {});

public:
    ChildWindow(Window& parent, const Vec2ui& size, const WindowSettings& settings = {});

	Window& parent() const { return *parent_; }
};

}