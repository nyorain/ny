#include <ny/window/window.hpp>

#include <ny/app/event.hpp>
#include <ny/app/app.hpp>
#include <ny/window/cursor.hpp>
#include <ny/backend/backend.hpp>
#include <ny/backend/windowContext.hpp>
#include <ny/draw/drawContext.hpp>
#include <ny/app/mouse.hpp>
#include <ny/app/keyboard.hpp>

#include <nytl/misc.hpp>
#include <nytl/log.hpp>

#include <iostream>
#include <climits>

namespace ny
{

namespace
{
int count = 0;
}

Window::Window()
{
}

Window::Window(const vec2ui& size, const WindowContextSettings& settings) 
	: maxSize_(UINT_MAX, UINT_MAX)
{
    create(size, settings);
}

Window::~Window()
{
}

void Window::create(const vec2ui& size, 
		const WindowContextSettings& settings)
{
	count++;
    size_ = size;

    if(!nyMainApp())
    {
		throw std::logic_error("Window::Window: Need an initialized app instance");
        return;
    }

	windowContext_ = nyMainApp()->backend().createWindowContext(*this, settings);
    hints_ |= windowContext_->additionalWindowHints();
}

void Window::close()
{
    destroyCallback_(*this);
    windowContext_.reset();
}

bool Window::processEvent(const Event& ev)
{
    if(EventHandler::processEvent(ev)) return 1;

    switch (ev.type())
    {
	case eventType::destroy:
		windowClose(static_cast<const DestroyEvent&>(ev));
		return true;
    case eventType::mouseButton:
        mouseButton(static_cast<const MouseButtonEvent&>(ev));
        return true;
    case eventType::mouseMove:
        mouseMove(static_cast<const MouseMoveEvent&>(ev));
        return true;
    case eventType::mouseCross:
        mouseCross(static_cast<const MouseCrossEvent&>(ev));
        return true;
    case eventType::mouseWheel:
        mouseWheel(static_cast<const MouseWheelEvent&>(ev));
        return true;
    case eventType::key:
        keyboardKey(static_cast<const KeyEvent&>(ev));
        return true;
    case eventType::windowFocus:
        windowFocus(static_cast<const FocusEvent&>(ev));
        return true;
    case eventType::windowSize:
        windowSize(static_cast<const SizeEvent&>(ev));
        return true;
    case eventType::windowPosition:
        windowPosition(static_cast<const PositionEvent&>(ev));
        return true;
    case eventType::windowDraw:
        windowDraw(static_cast<const DrawEvent&>(ev));
        return true;
    case eventType::windowShow:
        windowShow(static_cast<const ShowEvent&>(ev));
        return true;
    case eventType::windowRefresh:
        refresh();
        return true;
    case eventType::context:
        windowContext_->processEvent(static_cast<const ContextEvent&>(ev));
        return true;

    default:
        return false;
    }
}

void Window::refresh()
{
    windowContext_->refresh();
}

void Window::size(const vec2ui& size)
{
    size_ = size;
    windowContext_->size(size, 1);

    resizeCallback_(*this, size_);
}

void Window::position(const vec2i& position)
{
    position_ = position;
    windowContext_->position(position_);

    moveCallback_(*this, position_);
}

void Window::move(const vec2i& delta)
{
    position_ += delta;
    windowContext_->position(position_);

    moveCallback_(*this, position_);
}

void Window::show()
{
    windowContext_->show();
    shown_ = 1;
}

void Window::hide()
{
    windowContext_->hide();
    shown_ = false;
}

void Window::toggleShow()
{
    if(shown()) windowContext_->hide();
    else windowContext_->show();
}

void Window::maxSize(const vec2ui& size)
{
    maxSize_ = size;
    windowContext_->maxSize(size);
}

void Window::minSize(const vec2ui& size)
{
    minSize_ = size;
    windowContext_->minSize(size);
}

//event callbacks
void Window::windowClose(const DestroyEvent& event)
{
	windowContext_.reset();
	destroyCallback_(*this);

	count--;
	if(count <= 0)
	{
		if(nyMainApp())
		{
			nyMainApp()->exit();
		}
	}
}
void Window::mouseMove(const MouseMoveEvent& e)
{
    mouseMoveCallback_(*this, e);
}
void Window::mouseCross(const MouseCrossEvent& e)
{
    mouseOver_ = e.entered;
    mouseCrossCallback_(*this, e);
}
void Window::mouseButton(const MouseButtonEvent& e)
{
    mouseButtonCallback_(*this, e);
}
void Window::mouseWheel(const MouseWheelEvent& e)
{
    mouseWheelCallback_(*this, e);
}
void Window::keyboardKey(const KeyEvent& e)
{
    keyCallback_(*this, e);
}
void Window::windowSize(const SizeEvent& e)
{
    size_ = e.size;
    windowContext_->size(size_, e.change);
    resizeCallback_.call(*this, size_);
}
void Window::windowPosition(const PositionEvent& e)
{
    position_ = e.position;
    windowContext_->position(position_, e.change);
    moveCallback_(*this, position_);
}
void Window::windowDraw(const DrawEvent&)
{
    auto& dc = windowContext_->beginDraw();
    draw(dc);
    windowContext_->finishDraw();
}
void Window::windowShow(const ShowEvent& e)
{
    showCallback_(*this, e);
}
void Window::windowFocus(const FocusEvent& e)
{
    focus_ = e.focusGained;
    focusCallback_(*this, e);
}

//draw
void Window::draw(DrawContext& dc)
{
    dc.clear(Color::white); //TODO
    drawCallback_(*this, dc);
}

//util
/*
std::vector<childWindow*> Window::getWindowChildren()
{
    std::vector<childWindow*> ret;

    for(auto child : getChildren())
    {
        childWindow* w = dynamic_cast<childWindow*>(child);
        if(w) ret.push_back(w);
    }

    return ret;
}

window* Window::getWindowAt(vec2i position)
{
    std::vector<childWindow*> vec = getWindowChildren();
    for(unsigned int i(0); i < vec.size(); i++)
    {
        window* w = vec[i];
        if(w->getPosition().x < position.x && w->getPosition().x + (int)w->getSize().x > position.x && w->getPosition().y < position.y && w->getPosition().y + (int)w->getSize().y > position.y)
        {
            return w->getWindowAt(position - w->getPosition());
        }
    }

    if(position.x > 0 && position.x < (int) getSize().x && position.y > 0 && position.y < (int) getSize().y)
    {
        return this;
    }

    return nullptr;
}
*/

void Window::cursor(const Cursor& curs)
{
    windowContext_->cursor(curs);
}

void Window::addWindowHints(unsigned long hints)
{
    hints &= ~hints_;
    hints_ |= hints;

    windowContext_->addWindowHints(hints);
}

void Window::removeWindowHints(unsigned int hints)
{
    hints &= hints_;
    hints_ &= ~hints;

    windowContext_->removeWindowHints(hints);
}

/*
void toplevelWindow::setIcon(const image* icon)
{
    if(!checkValid()) return;
    getWindowContext()->setIcon(icon);
}

void toplevelWindow::mouseButton(const mouseButtonEvent& ev)
{
    window::mouseButton(ev);

/*
    if(!isCustomResized() || !hasResizeHint())
        return;
* /

    windowEdge medge = windowEdge::Unknown;

    int length = 100;

    bool found = 0;

    if(ev.position.y > (int) size_.y - length)
    {
        medge = windowEdge::Bottom;

        found = 1;
    }

    else if(ev.position.y < length)
    {
        medge = windowEdge::Top;

        found = 1;
    }

    if(ev.position.x > (int) size_.x - length)
    {
        if(medge == windowEdge::Top) medge = windowEdge::TopRight;
        else if(medge == windowEdge::Bottom) medge = windowEdge::BottomRight;
        else medge = windowEdge::Right;

        found = 1;
    }

    if(ev.position.x < length)
    {
        if(medge == windowEdge::Top) medge = windowEdge::TopLeft;
        else if(medge == windowEdge::Bottom) medge = windowEdge::BottomLeft;
        else medge = windowEdge::Left;

        found = 1;
    }

    if(found) getWindowContext()->beginResize(&ev, medge);
    else getWindowContext()->beginMove(&ev);
}

void toplevelWindow::mouseMove(const mouseMoveEvent& ev)
{
    window::mouseMove(ev);

/*
    if(!isCustomResized() || !hasResizeHint())
        return;
* /
    cursorType t = cursorType::grab;

    int length = 100;

    if(ev.position.y > (int) size_.y - length)
    {
        t = cursorType::sizeBottom;
    }

    else if(ev.position.y < length)
    {
        t = cursorType::sizeTop;
    }

    if(ev.position.x > (int) size_.x - length)
    {
        if(t == cursorType::sizeTop) t = cursorType::sizeTopRight;
        else if(t == cursorType::sizeBottom) t = cursorType::sizeBottomRight;
        else t = cursorType::sizeRight;
    }

    if(ev.position.x < length)
    {
        if(t == cursorType::sizeTop) t = cursorType::sizeTopLeft;
        else if(t == cursorType::sizeBottom) t = cursorType::sizeBottomLeft;
        else t = cursorType::sizeLeft;
    }

    cursor_.fromNativeType(t);
    windowContext_->updateCursor(nullptr);
}

void toplevelWindow::setTitle(const std::string& n)
{
    title_ = n;
    if(!checkValid()) return;

    getWindowContext()->setTitle(n);
};

/*
bool toplevelWindow::setCustomDecorated(bool set)
{
    hints_ |= windowHints::CustomDecorated;

    if(!checkValid()) return 0;

    if(set)windowContext_->addWindowHints(windowHints::CustomDecorated);
    else windowContext_->removeWindowHints(windowHints::CustomDecorated);

    return 1; //todo: this ans 2 following: correct return, if it was successful
}

bool toplevelWindow::setCustomResized(bool set)
{
    hints_ |= windowHints::CustomResized;

    if(!checkValid()) return 0;

    if(set)windowContext_->addWindowHints(windowHints::CustomResized);
    else windowContext_->removeWindowHints(windowHints::CustomResized);

    return 1;
}

bool toplevelWindow::setCustomMoved(bool set)
{
    hints_ |= windowHints::CustomMoved;

    if(!checkValid()) return 0;

    if(set)windowContext_->addWindowHints(windowHints::CustomMoved);
    else windowContext_->removeWindowHints(windowHints::CustomMoved);

    return 1;
}
//*,///
void toplevelWindow::setMaximizeHint(bool hint)
{
    hints_ |= windowHints::Maximize;

    if(!checkValid()) return;

    if(hint)windowContext_->addWindowHints(windowHints::Maximize);
    else windowContext_->removeWindowHints(windowHints::Maximize);
}

void toplevelWindow::setMinimizeHint(bool hint)
{
    hints_ |= windowHints::Minimize;

    if(!checkValid()) return;

    if(hint)windowContext_->addWindowHints(windowHints::Minimize);
    else windowContext_->removeWindowHints(windowHints::Minimize);
}

void toplevelWindow::setResizeHint(bool hint)
{
    hints_ |= windowHints::Resize;

    if(!checkValid()) return;

    if(hint)windowContext_->addWindowHints(windowHints::Resize);
    else windowContext_->removeWindowHints(windowHints::Resize);
}

void toplevelWindow::setMoveHint(bool hint)
{
    hints_ |= windowHints::Move;

    if(!checkValid()) return;

    if(hint)windowContext_->addWindowHints(windowHints::Move);
    else windowContext_->removeWindowHints(windowHints::Move);
}

void toplevelWindow::setCloseHint(bool hint)
{
    hints_ |= windowHints::Close;

    if(!checkValid()) return;

    if(hint)windowContext_->addWindowHints(windowHints::Close);
    else windowContext_->removeWindowHints(windowHints::Close);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//childWindow///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
childWindow::childWindow() : window()
{
}

childWindow::childWindow(window& parent, vec2i position, vec2ui size, windowContextSettings settings) : window()
{
    create(parent, position, size, settings);
}

void childWindow::create(window& parent, vec2i position, vec2ui size, windowContextSettings settings)
{
    //hints_ |= windowHints::Child;
    window::create(parent, position, size, settings);
}

bool childWindow::isVirtual() const
{
    if(!checkValid()) return 0;
    return windowContext_->isVirtual();
}
*/

}
