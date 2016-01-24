#include <ny/window/toplevel.hpp>
#include <ny/backend/windowContext.hpp>

namespace ny
{

ToplevelWindow::ToplevelWindow(const vec2ui& size, const std::string& title, 
		const WindowSettings& settings) : Window(), title_(title)
{
	create(size, title, settings);
}

ToplevelWindow::~ToplevelWindow()
{
}

void ToplevelWindow::create(const vec2ui& size, const std::string& title, 
		const WindowSettings& settings)
{
	hints_ |= windowHints::maximize | windowHints::minimize | windowHints::close | 
		windowHints::showInTaskbar | windowHints::move | windowHints::resize;

	title_ = title;
	Window::create(size, settings);
}

void ToplevelWindow::maximizeHint(bool hint)
{
	if(hint) addWindowHints(windowHints::maximize);
	else removeWindowHints(windowHints::maximize);
}

}
