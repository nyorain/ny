#include <ny/window/toplevel.hpp>
#include <ny/app/mouse.hpp>
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

bool ToplevelWindow::customDecorated(bool hint)
{
	if(hint) addWindowHints(windowHints::customDecorated);
	else removeWindowHints(windowHints::customDecorated);

	return 1;
}

void ToplevelWindow::mouseMoveEvent(const MouseMoveEvent& ev)
{
    Window::mouseMoveEvent(ev);
    if(!customDecorated()) return;

	Cursor::Type t = Cursor::Type::grab;

    int length = 100;

    if(ev.position.y > (int) size_.y - length)
    {
        t = Cursor::Type::sizeBottom;
    }

    else if(ev.position.y < length)
    {
        t = Cursor::Type::sizeTop;
    }

    if(ev.position.x > (int) size_.x - length)
    {
        if(t == Cursor::Type::sizeTop) t = Cursor::Type::sizeTopRight;
        else if(t == Cursor::Type::sizeBottom) t = Cursor::Type::sizeBottomRight;
        else t = Cursor::Type::sizeRight;
    }

    if(ev.position.x < length)
    {
        if(t == Cursor::Type::sizeTop) t = Cursor::Type::sizeTopLeft;
        else if(t == Cursor::Type::sizeBottom) t = Cursor::Type::sizeBottomLeft;
        else t = Cursor::Type::sizeLeft;
    }

    cursor_.nativeType(t);
	windowContext_->cursor(cursor_);
}

void ToplevelWindow::mouseButtonEvent(const MouseButtonEvent& ev)
{
    Window::mouseButtonEvent(ev);
    if(!customDecorated()) return;

    WindowEdge medge = WindowEdge::unknown;

    int length = 100;

    bool found = 0;

    if(ev.position.y > (int) size_.y - length)
    {
        medge = WindowEdge::bottom;

        found = 1;
    }

    else if(ev.position.y < length)
    {
        medge = WindowEdge::top;

        found = 1;
    }

    if(ev.position.x > (int) size_.x - length)
    {
        if(medge == WindowEdge::top) medge = WindowEdge::topRight;
        else if(medge == WindowEdge::bottom) medge = WindowEdge::bottomRight;
        else medge = WindowEdge::right;

        found = 1;
    }

    if(ev.position.x < length)
    {
        if(medge == WindowEdge::top) medge = WindowEdge::topLeft;
        else if(medge == WindowEdge::bottom) medge = WindowEdge::bottomLeft;
        else medge = WindowEdge::left;

        found = 1;
    }

    if(found) windowContext()->beginResize(&ev, medge);
    else windowContext()->beginMove(&ev);
}

}
