#include <ny/window/toplevel.hpp>

namespace ny
{

ToplevelWindow::ToplevelWindow(const vec2ui& size, const std::string& title, 
		const WindowContextSettings& settings) : Window(), title_(title)
{
	Window::create(size, settings);
}

ToplevelWindow::~ToplevelWindow()
{
}

void ToplevelWindow::create(const vec2ui& size, const std::string& title, 
		const WindowContextSettings& settings)
{
	title_ = title;
	Window::create(size, settings);
}

void ToplevelWindow::draw(DrawContext& dc)
{
	Window::draw(dc);
}

}
