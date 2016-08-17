#include <ny/app/window.hpp>
#include <ny/app/events.hpp>
#include <ny/app/app.hpp>
#include <ny/app/mouse.hpp>
#include <ny/app/keyboard.hpp>

#include <ny/base/cursor.hpp>
#include <ny/base/event.hpp>
#include <ny/base/log.hpp>

#include <ny/backend/windowContext.hpp>
#include <ny/backend/appContext.hpp>

#include <nytl/misc.hpp>
#include <evg/drawContext.hpp>

#include <iostream>
#include <climits>

namespace ny
{

Window::Window()
{
}

Window::Window(App& app, const Vec2ui& size, const WindowSettings& settings)
	: maxSize_(UINT_MAX, UINT_MAX)
{
    create(app, size, settings);
}

Window::~Window()
{
}

void Window::create(App& papp, const Vec2ui& size, const WindowSettings& settings)
{
	app_ = &papp;
    size_ = size;

	auto cpy = settings;
	cpy.size = size;

	windowContext_ = app().appContext().createWindowContext(cpy);

	windowContext_->eventHandler(*this);
	app().windowCreated();
}

void Window::close()
{
    onClose(*this);
    windowContext_.reset();

	app().windowClosed();
}

bool Window::handleEvent(const Event& ev)
{
    switch(ev.type())
    {
	case eventType::windowClose:
		closeEvent(static_cast<const CloseEvent&>(ev));
		return true;
    case eventType::mouseButton:
        mouseButtonEvent(static_cast<const MouseButtonEvent&>(ev));
        return true;
    case eventType::mouseMove:
        mouseMoveEvent(static_cast<const MouseMoveEvent&>(ev));
        return true;
    case eventType::mouseCross:
        mouseCrossEvent(static_cast<const MouseCrossEvent&>(ev));
        return true;
    case eventType::mouseWheel:
        mouseWheelEvent(static_cast<const MouseWheelEvent&>(ev));
        return true;
    case eventType::key:
        keyEvent(static_cast<const KeyEvent&>(ev));
        return true;
    case eventType::windowFocus:
        focusEvent(static_cast<const FocusEvent&>(ev));
        return true;
    case eventType::windowSize:
        sizeEvent(static_cast<const SizeEvent&>(ev));
        return true;
    case eventType::windowPosition:
        positionEvent(static_cast<const PositionEvent&>(ev));
        return true;
    case eventType::windowDraw:
        drawEvent(static_cast<const DrawEvent&>(ev));
        return true;
    case eventType::windowShow:
        showEvent(static_cast<const ShowEvent&>(ev));
        return true;
    case eventType::windowRefresh:
        refresh();
        return true;

    default:
        return false;
    }
}

void Window::refresh()
{
    windowContext_->refresh();
	//app().dispatcher().dispatch(std::make_unique<DrawEvent>(this));
}

void Window::size(const Vec2ui& size)
{
    size_ = size;
    windowContext_->size(size);

    onResize(*this, size_);
}

void Window::position(const Vec2i& position)
{
    position_ = position;
    windowContext_->position(position_);

	onMove(*this, position_);
}

void Window::move(const Vec2i& delta)
{
    position_ += delta;
    windowContext_->position(position_);

    onMove(*this, position_);
}

void Window::show()
{
    windowContext_->show();
    shown_ = true;
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

void Window::maxSize(const Vec2ui& size)
{
    maxSize_ = size;
    windowContext_->maxSize(size);
}

void Window::minSize(const Vec2ui& size)
{
    minSize_ = size;
    windowContext_->minSize(size);
}

void Window::draw(DrawContext& dc)
{
    onDraw(*this, dc);
}

//event Callbacks
void Window::closeEvent(const CloseEvent&)
{
	close();
}
void Window::mouseMoveEvent(const MouseMoveEvent& e)
{
	onMouseMove(*this, e);
}
void Window::mouseCrossEvent(const MouseCrossEvent& e)
{
    mouseOver_ = e.entered;
	onMouseCross(*this, e);
}
void Window::mouseButtonEvent(const MouseButtonEvent& e)
{
    onMouseButton(*this, e);
}
void Window::mouseWheelEvent(const MouseWheelEvent& e)
{
    onMouseWheel(*this, e);
}
void Window::keyEvent(const KeyEvent& e)
{
    onKey(*this, e);
}
void Window::sizeEvent(const SizeEvent& e)
{
    size_ = e.size;
    onResize(*this, size_);
	refresh();
}
void Window::positionEvent(const PositionEvent& e)
{
    position_ = e.position;
    onMove(*this, position_);
}
void Window::drawEvent(const DrawEvent&)
{
    auto guard = windowContext_->draw();
    draw(guard.dc());
}
void Window::showEvent(const ShowEvent& e)
{
    onShow(*this, e);
}
void Window::focusEvent(const FocusEvent& e)
{
    focus_ = e.focus;
    onFocus(*this, e);
}

void Window::cursor(const Cursor& curs)
{
    windowContext_->cursor(curs);
}

NativeWindowHandle Window::nativeHandle() const
{
	return windowContext_->nativeHandle();
}

/*
void toplevelWindow::setTitle(const std::string& n)
{
    title_ = n;
    if(!checkValid()) return;

    getWindowContext()->setTitle(n);
};

bool toplevelWindow::setCustomDecorated(bool set)
{
    hints_ |= windowHints::CustomDecorated;

    if(!checkValid()) return 0;

    if(set)windowContext_->addWindowHints(windowHints::CustomDecorated);
    else windowContext_->removeWindowHints(windowHints::CustomDecorated);

    return 1; //todo: this ans 2 following: corRect return, if it was successful
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

}
*/

}
