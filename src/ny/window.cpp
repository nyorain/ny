#include <ny/window.hpp>

#include <ny/event.hpp>
#include <ny/app.hpp>
#include <ny/error.hpp>
#include <ny/cursor.hpp>
#include <ny/backend.hpp>
#include <ny/windowContext.hpp>
#include <ny/drawContext.hpp>
#include <ny/style.hpp>
#include <ny/mouse.hpp>
#include <ny/keyboard.hpp>

#include <nyutil/misc.hpp>

#include <iostream>
#include <climits>

namespace ny
{

//wcSettings
windowContextSettings::~windowContextSettings()
{
}

//window//////////////////////////////////////////////////////////////////////////////
window::window() : eventHandler(), surface(), position_(0,0), minSize_(0,0), maxSize_(UINT_MAX, UINT_MAX), focus_(0), valid_(0), mouseOver_(0), windowContext_(nullptr)
{
}

window::window(eventHandler& parent, vec2ui position, vec2ui size, const windowContextSettings& settings) : eventHandler(), surface(), position_(0,0), minSize_(0,0), maxSize_(UINT_MAX, UINT_MAX), focus_(0), valid_(0), mouseOver_(0), windowContext_(nullptr)
{
    create(parent, size, position);
}

window::~window()
{
}

void window::create(eventHandler& parent, vec2i position, vec2ui size, const windowContextSettings& settings)
{
    size_ = size;
    position_ = position;

    if(!nyMainApp() || !nyMainApp()->getBackend())
    {
        nyError("window::create: window can only be created when mainApp exists and is initialized");
        return;
    }

    std::unique_ptr<windowContext> newWC;
    try
    {
        eventHandler::create(parent);
        newWC = createWindowContext(*this, settings);
    }
    catch(const std::exception& err)
    {
        newWC.reset();
        nyError(err);
        return;
    }

    if(!newWC.get())
    {
        nyError("window::create: failed to create windowContext");
        return;
    }

    hints_ |= newWC->getAdditionalWindowHints();

    windowContext_ = std::move(newWC);
    valid_ = 1;
}


void window::destroy()
{

}


bool window::processEvent(std::unique_ptr<event> ev)
{
    if(!valid_)
        return 0;

    bool ret = false;

    switch (ev->type())
    {
        /*
    case eventType::mouseButton:
        mouseButton(ev.to<mouseButtonEvent>());
        return true;
    case eventType::mouseMove:
        mouseMove(ev.to<mouseMoveEvent>());
        return true;
    case eventType::mouseCross:
        mouseCross(ev.to<mouseCrossEvent>());
        return true;
    case eventType::mouseWheel:
        mouseWheel(ev.to<mouseWheelEvent>());
        return true;
    case eventType::key:
        keyboardKey(ev.to<keyEvent>());
        return true;
    case eventType::windowFocus:
        windowFocus(ev.to<focusEvent>());
        return true;
    case eventType::windowSize:
        windowSize(ev.to<sizeEvent>());
        return true;
    case eventType::windowPosition:
        windowPosition(ev.to<positionEvent>());
        return true;
    case eventType::windowDraw:
        windowDraw(ev.to<drawEvent>());
        return true;
    case eventType::context:
        windowContext_->sendContextEvent(ev.to<contextEvent>());
        return true;
*/
    default:
        return false;
    }
}

void window::refresh()
{
    if(!valid_)
        return;

    windowContext_->refresh();
}

void window::setSize(vec2ui size)
{
    if(!valid_)
        return;

    size_ = size;
    windowContext_->setSize(size, 1);

    resizeCallback_(*this, size_);
}

void window::setPosition(vec2i position)
{
    if(!valid_) return;

    position_ = position;
    windowContext_->setPosition(position_);

    moveCallback_(*this, position_);
}

void window::move(vec2i delta)
{
    if(!valid_) return;

    position_ += delta;
    windowContext_->setPosition(position_);

    moveCallback_(*this, position_);
}

void window::show()
{
    if(!valid_) return;

    windowContext_->show();
    shown_ = 1;
}

void window::hide()
{
    if(!valid_) return;

    windowContext_->hide();
    shown_ = false;
}

void window::toggleShow()
{
    if(!valid_) return;

    if(isShown())
        windowContext_->hide();

    else
        windowContext_->show();
}

void window::setMaxSize(vec2ui size)
{
    maxSize_ = size;
    windowContext_->setMaxSize(size);
}

void window::setMinSize(vec2ui size)
{
    minSize_ = size;
    windowContext_->setMinSize(size);
}

void window::windowSize(sizeEvent& e)
{
    size_ = e.size;
    windowContext_->setSize(size_, e.change);

    resizeCallback_.call(*this, size_);
}

void window::windowPosition(positionEvent& e)
{
    position_ = e.position;
    windowContext_->setPosition(position_, 0);

    moveCallback_(*this, position_);
}

void window::windowDraw(drawEvent& e)
{
    drawContext& dc = windowContext_->beginDraw();
    draw(dc);
    windowContext_->finishDraw();
 }

void window::draw(drawContext& dc)
{
    drawCallback_(*this, dc);

    std::vector<childWindow*> wvec = getWindowChildren();
    for(unsigned int i(0); i < wvec.size(); i++)
    {
        if(wvec[i]->isVirtual())
        {
            drawEvent e;
            e.handler = wvec[i];
            wvec[i]->windowDraw(e);
        }
    }
}

void window::windowDestroy(destroyEvent& e)
{
    windowContext_.reset();

    eventHandler::destroy();
    destroyCallback_(*this, e);
    valid_ = 0;
}

void window::windowShow(showEvent& e)
{

}

void window::windowFocus(focusEvent& e)
{
    if(e.focusGained)
    {
        focus_ = 1;
    }
    else
    {
        focus_ = 0;
    }

    focusCallback_(*this, e);
}

std::vector<childWindow*> window::getWindowChildren()
{
    std::vector<childWindow*> ret;

    for(auto child : getChildren())
    {
        childWindow* w = dynamic_cast<childWindow*>(child);
        if(w) ret.push_back(w);
    }

    return ret;
}

window* window::getWindowAt(vec2i position)
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

void window::setCursor(const cursor& curs)
{
    windowContext_->setCursor(curs);
}

void window::addWindowHints(unsigned long hints)
{
    hints &= ~hints_; //remove all the hints, which are still set to make it easier for windowContext

    hints_ |= hints;

    if(!valid_)
        return;

    windowContext_->addWindowHints(hints);
}

void window::removeWindowHints(unsigned int hints)
{
    hints &= hints_; //remove all the hints, which are not set and cant be removed to make it easier for windowContext

    hints_ &= ~hints;

    if(!valid_)
        return;

    windowContext_->removeWindowHints(hints);
}

///////////////////////////////////////////////////////////////////////////////////
void window::mouseMove(mouseMoveEvent& e)
{
    mouseMoveCallback_(*this, e);
}

void window::mouseCross(mouseCrossEvent& e)
{
    if(e.entered)
    {
        windowContext_->updateCursor(&e);
        mouseOver_ = 1;
    }
    else
    {
        mouseOver_ = 0;
    }

    mouseCrossCallback_(*this, e);
}

void window::mouseButton(mouseButtonEvent& e)
{
    mouseButtonCallback_(*this, e);
}

void window::mouseWheel(mouseWheelEvent& e)
{
    mouseWheelCallback_(*this, e);
}

void window::keyboardKey(keyEvent& e)
{
    keyCallback_(*this, e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//toplevelwindow///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
toplevelWindow::toplevelWindow() : window()
{
}

toplevelWindow::toplevelWindow(vec2i position, vec2ui size, std::string title, const windowContextSettings& settings) : window()
{
    create(position, size, title, settings);
}

void toplevelWindow::create(vec2i position, vec2ui size, std::string title, const windowContextSettings& settings)
{
    title_ = title;

    hints_ |= windowHints::Toplevel;
    hints_ |= windowHints::Resize;
	hints_ |= windowHints::Move;

    window::create(*nyMainApp(), position, size, settings);

    cursor c;
    c.fromNativeType(cursorType::leftPtr);
    windowContext_->setCursor(c);
}

void toplevelWindow::setIcon(const image* icon)
{
    getWindowContext()->setIcon(icon);
}

void toplevelWindow::mouseButton(mouseButtonEvent& ev)
{
    window::mouseButton(ev);


    if(!isCustomResized() || !hasResizeHint())
        return;


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

void toplevelWindow::mouseMove(mouseMoveEvent& ev)
{
    window::mouseMove(ev);

    if(!isCustomResized() || !hasResizeHint())
        return;

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
     if(valid_) getWindowContext()->setTitle(n);
};

bool toplevelWindow::setCustomDecorated(bool set)
{
    if(!valid_)
        return 0;

    hints_ |= windowHints::CustomDecorated;

    if(set)windowContext_->addWindowHints(windowHints::CustomDecorated);
    else windowContext_->removeWindowHints(windowHints::CustomDecorated);

    return 1; //todo: this ans 2 following: correct return, if it was successful
}

bool toplevelWindow::setCustomResized(bool set)
{
    if(!valid_)
        return 0;

    hints_ |= windowHints::CustomResized;

    if(set)windowContext_->addWindowHints(windowHints::CustomResized);
    else windowContext_->removeWindowHints(windowHints::CustomResized);

    return 1;
}

bool toplevelWindow::setCustomMoved(bool set)
{
    if(!valid_)
        return 0;

    hints_ |= windowHints::CustomMoved;

    if(set)windowContext_->addWindowHints(windowHints::CustomMoved);
    else windowContext_->removeWindowHints(windowHints::CustomMoved);

    return 1;
}

void toplevelWindow::setMaximizeHint(bool hint)
{
    if(!valid_)
        return;

    hints_ |= windowHints::Maximize;

    if(hint)windowContext_->addWindowHints(windowHints::Maximize);
    else windowContext_->removeWindowHints(windowHints::Maximize);
}

void toplevelWindow::setMinimizeHint(bool hint)
{
    if(!valid_)
        return;

    hints_ |= windowHints::Minimize;

    if(hint)windowContext_->addWindowHints(windowHints::Minimize);
    else windowContext_->removeWindowHints(windowHints::Minimize);
}

void toplevelWindow::setResizeHint(bool hint)
{
    if(!valid_)
        return;

    hints_ |= windowHints::Resize;

    if(hint)windowContext_->addWindowHints(windowHints::Resize);
    else windowContext_->removeWindowHints(windowHints::Resize);
}

void toplevelWindow::setMoveHint(bool hint)
{
    if(!valid_)
        return;

    hints_ |= windowHints::Move;

    if(hint)windowContext_->addWindowHints(windowHints::Move);
    else windowContext_->removeWindowHints(windowHints::Move);
}

void toplevelWindow::setCloseHint(bool hint)
{
    if(!valid_)
        return;

    hints_ |= windowHints::Close;

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
    hints_ |= windowHints::Child;
    window::create(parent, position, size, settings);
}

bool childWindow::isVirtual() const
{
    if(!valid_)
        return 0;

    return windowContext_->isVirtual();
}

}
