#include <ny/window.hpp>

#include <ny/event.hpp>
#include <ny/app.hpp>
#include <ny/error.hpp>
#include <ny/cursor.hpp>
#include <ny/backend.hpp>
#include <ny/windowContext.hpp>
#include <ny/drawContext.hpp>
#include <ny/style.hpp>

#include <ny/util/misc.hpp>

#include <iostream>
#include <climits>

namespace ny
{

windowContextSettings::~windowContextSettings()
{
}

//window//////////////////////////////////////////////////////////////////////////////
window::window() : eventHandler(), surface(), position_(0,0), minSize_(0,0), maxSize_(UINT_MAX, UINT_MAX), focus_(0), valid_(0), mouseOver_(0), windowContext_(nullptr)
{
}

window::window(eventHandler* parent, vec2ui position, vec2ui size, const windowContextSettings& settings) : eventHandler(), surface(), position_(0,0), minSize_(0,0), maxSize_(UINT_MAX, UINT_MAX), focus_(0), valid_(0), mouseOver_(0), windowContext_(nullptr)
{
    create(parent, size, position);
}

window::~window()
{
    close();
}

void window::create(eventHandler* parent, vec2i position, vec2ui size, const windowContextSettings& settings)
{
    size_ = size;
    position_ = position;

    if(!getMainApp() || !getMainApp()->getBackend())
    {
        sendError("window::create: window can only be created when mainApp exists and is initialized");
        return;
    }

    if(!parent)
    {
        sendError("window::create: invalid parent");
        return;
    }

    windowContext* newWC = nullptr;
    try
    {
        eventHandler::create(*parent);
        newWC = createWindowContext(*this, settings);
    }
    catch(const std::exception& err)
    {
        if(newWC) delete newWC;
        sendError(err);
        return;
    }

    if(!newWC)
    {
        sendError("window::create: failed to create windowContext");
        return;
    }

    hints_ |= newWC->getAdditionalWindowHints();

    windowContext_ = newWC;
    valid_ = 1;
}


void window::close()
{
    if(!valid_)
        return;

    destroyEvent e;

    e.backend = 0;
    e.handler = this;

    getMainApp()->destroyHandler(e);
}


bool window::processEvent(event& ev)
{
    if(!valid_)
        return 0;

    bool ret = false;

    switch (ev.type)
    {
    case eventType::mouseButton:
        mouseButton(ev.to<mouseButtonEvent>());
        ret = true;
        break;
    case eventType::mouseMove:
        mouseMove(ev.to<mouseMoveEvent>());
        ret = true;
        break;
    case eventType::mouseCross:
        mouseCross(ev.to<mouseCrossEvent>());
        ret = true;
        break;
    case eventType::mouseWheel:
        mouseWheel(ev.to<mouseWheelEvent>());
        ret = true;
        break;
    case eventType::key:
        keyboardKey(ev.to<keyEvent>());
        ret = true;
        break;
    case eventType::windowFocus:
        windowFocus(ev.to<focusEvent>());
        ret = true;
        break;
    case eventType::windowSize:
        windowSize(ev.to<sizeEvent>());
        ret = true;
        break;
    case eventType::windowPosition:
        windowPosition(ev.to<positionEvent>());
        ret = true;
        break;
    case eventType::windowDraw:
        windowDraw(ev.to<drawEvent>());
        ret = true;
        break;
    case eventType::destroy:
        windowDestroy(ev.to<destroyEvent>());
        ret = true;
        break;
    case eventType::context:
        windowContext_->sendContextEvent(ev.to<contextEvent>());
        ret = true;
        break;

    default:
        ret = false;
    }

    return ret;
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
    if(e.backend == 0)windowContext_->setSize(size_, 1);
    else windowContext_->setSize(size_, 0); //only informs the windowContext

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
            e.backend = 0;
            wvec[i]->windowDraw(e);
        }
    }
}

void window::windowDestroy(destroyEvent& e)
{
    delete windowContext_;
    eventHandler::destroy();

    destroyCallback_(*this, e);

    valid_ = 0;
}

void window::windowShow(showEvent& e)
{

}

void window::windowFocus(focusEvent& e)
{
    if(e.state == focusState::gained)
    {
        focus_ = 1;
    }

    focusCallback_(*this, e);
}

std::vector<childWindow*> window::getWindowChildren()
{
    std::vector<childWindow*> ret;

    for(size_t i(0); i < children_.size(); i++)
    {
        childWindow* w = dynamic_cast<childWindow*>(children_[i]);
        if(w)
        {
            ret.push_back(w);
        }
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


connection& window::onDraw(std::function<void(window&, drawContext&)> func)
{
    return drawCallback_.add(func);
};
connection& window::onResize(std::function<void(window&, const vec2ui&)> func)
{
    return resizeCallback_.add(func);
};
connection& window::onMove(std::function<void(window&, const vec2i&)> func)
{
    return moveCallback_.add(func);
};
connection& window::onDestroy(std::function<void(window&, const destroyEvent&)> func)
{
    return destroyCallback_.add(func);
};
connection& window::onFocus(std::function<void(window&, const focusEvent&)> func)
{
    return focusCallback_.add(func);
};
connection& window::onMouseMove(std::function<void(window&, const mouseMoveEvent&)> func)
{
    return mouseMoveCallback_.add(func);
};
connection& window::onMouseButton(std::function<void(window&, const mouseButtonEvent&)> func)
{
    return mouseButtonCallback_.add(func);
};
connection& window::onMouseCross(std::function<void(window&, const mouseCrossEvent&)> func)
{
    return mouseCrossCallback_.add(func);
};
connection& window::onMouseWheel(std::function<void(window&, const mouseWheelEvent&)> func)
{
    return mouseWheelCallback_.add(func);
};
connection& window::onKey(std::function<void(window&, const keyEvent&)> func)
{
    return keyCallback_.add(func);
};
//////////////////////////////////////////////////////////////////////////////////////
connection& window::onDraw(std::function<void(drawContext&)> func)
{
    return drawCallback_.add([func](window&, drawContext& a)
                                {
                                    func(a);
                                });
};
connection& window::onResize(std::function<void(const vec2ui&)> func)
{
    return resizeCallback_.add([func](window&, const vec2ui& a)
                                {
                                    func(a);
                                });
};
connection& window::onMove(std::function<void(const vec2i&)> func)
{
    return moveCallback_.add([func](window&, const vec2i& a)
                                {
                                    func(a);
                                });
};
connection& window::onDestroy(std::function<void(const destroyEvent&)> func)
{
    return destroyCallback_.add([func](window&, const destroyEvent& a)
                                {
                                    func(a);
                                });
};
connection& window::onFocus(std::function<void(const focusEvent&)> func)
{
    return focusCallback_.add([func](window&, const focusEvent& a)
                                {
                                    func(a);
                                });
};
connection& window::onMouseMove(std::function<void(const mouseMoveEvent&)> func)
{
    return mouseMoveCallback_.add([func](window&, const mouseMoveEvent& a)
                                {
                                    func(a);
                                });
};
connection& window::onMouseButton(std::function<void(const mouseButtonEvent&)> func)
{
    return mouseButtonCallback_.add([func](window&, const mouseButtonEvent& a)
                                {
                                    func(a);
                                });
};
connection& window::onMouseCross(std::function<void(const mouseCrossEvent&)> func)
{
    return mouseCrossCallback_.add([func](window&, const mouseCrossEvent& a)
                                {
                                    func(a);
                                });
};
connection& window::onMouseWheel(std::function<void(const mouseWheelEvent&)> func)
{
    return mouseWheelCallback_.add([func](window&, const mouseWheelEvent& a)
                                {
                                    func(a);
                                });
};
connection& window::onKey(std::function<void(const keyEvent&)> func)
{
    return keyCallback_.add([func](window&, const keyEvent& a)
                                {
                                    func(a);
                                });
};
///////////////////////////////////////////////////////////////////////////////
connection& window::onDraw(std::function<void(window&)> func)
{
    return drawCallback_.add([func](window& w, drawContext&)
                                {
                                    func(w);
                                });
};
connection& window::onResize(std::function<void(window&)> func)
{
    return resizeCallback_.add([func](window& w, const vec2ui&)
                                {
                                    func(w);
                                });
};
connection& window::onMove(std::function<void(window&)> func)
{
    return moveCallback_.add([func](window& w, const vec2i&)
                                {
                                    func(w);
                                });
};
connection& window::onDestroy(std::function<void(window&)> func)
{
    return destroyCallback_.add([func](window& w, const destroyEvent&)
                                {
                                    func(w);
                                });
};
connection& window::onFocus(std::function<void(window&)> func)
{
    return focusCallback_.add([func](window& w, const focusEvent&)
                                {
                                    func(w);
                                });
};
connection& window::onMouseMove(std::function<void(window&)> func)
{
    return mouseMoveCallback_.add([func](window& w, const mouseMoveEvent&)
                                {
                                    func(w);
                                });
};
connection& window::onMouseButton(std::function<void(window&)> func)
{
    return mouseButtonCallback_.add([func](window& w, const mouseButtonEvent&)
                                {
                                    func(w);
                                });
};
connection& window::onMouseCross(std::function<void(window&)> func)
{
    return mouseCrossCallback_.add([func](window& w, const mouseCrossEvent&)
                                {
                                    func(w);
                                });
};
connection& window::onMouseWheel(std::function<void(window&)> func)
{
    return mouseWheelCallback_.add([func](window& w, const mouseWheelEvent&)
                                {
                                    func(w);
                                });
};
connection& window::onKey(std::function<void(window&)> func)
{
    return keyCallback_.add([func](window& w, const keyEvent&)
                                {
                                    func(w);
                                });
};

///////////////////////////////////////////////////////////////////////////////////////
connection& window::onDraw(std::function<void()> func)
{
    return drawCallback_.add([func](window&, const drawContext&)
                                {
                                    func();
                                });
};
connection& window::onResize(std::function<void()> func)
{
    return resizeCallback_.add([func](window&, const vec2ui)
                                {
                                    func();
                                });
};
connection& window::onMove(std::function<void()> func)
{
    return moveCallback_.add([func](window&, const vec2i)
                                {
                                    func();
                                });
};
connection& window::onDestroy(std::function<void()> func)
{
    return destroyCallback_.add([func](window&, const destroyEvent&)
                                {
                                    func();
                                });
};
connection& window::onFocus(std::function<void()> func)
{
    return focusCallback_.add([func](window&, const focusEvent&)
                                {
                                    func();
                                });
};
connection& window::onMouseMove(std::function<void()> func)
{
    return mouseMoveCallback_.add([func](window&, const mouseMoveEvent&)
                                {
                                    func();
                                });
};
connection& window::onMouseButton(std::function<void()> func)
{
    return mouseButtonCallback_.add([func](window&, const mouseButtonEvent&)
                                {
                                    func();
                                });
};
connection& window::onMouseCross(std::function<void()> func)
{
    return mouseCrossCallback_.add([func](window&, const mouseCrossEvent&)
                                {
                                    func();
                                });
};
connection& window::onMouseWheel(std::function<void()> func)
{
    return mouseWheelCallback_.add([func](window&, const mouseWheelEvent&)
                                {
                                    func();
                                });
};
connection& window::onKey(std::function<void()> func)
{
    return keyCallback_.add([func](window&, const keyEvent&)
                                {
                                    func();
                                });
};

///////////////////////////////////////////////////////////////////////////////////
void window::mouseMove(mouseMoveEvent& e)
{
    mouseMoveCallback_(*this, e);
}

void window::mouseCross(mouseCrossEvent& e)
{
    if(e.state == crossType::entered)
    {
        windowContext_->updateCursor(&e);
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

toplevelWindow::toplevelWindow(vec2i position, vec2ui size, std::string name, const windowContextSettings& settings) : window()
{
    create(position, size, name, settings);
}

void toplevelWindow::create(vec2i position, vec2ui size, std::string name, const windowContextSettings& settings)
{
    name_ = name;

    hints_ |= windowHints::Toplevel;
    hints_ |= windowHints::Resize;
	hints_ |= windowHints::Move;

    window::create(getMainApp(), position, size, settings);

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

void toplevelWindow::setName(std::string n)
{
     name_ = n;

     if(valid_) getWindowContext()->setName(n);
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

childWindow::childWindow(window* parent, vec2i position, vec2ui size, windowContextSettings settings) : window()
{
    create(parent, position, size, settings);
}

void childWindow::create(window* parent, vec2i position, vec2ui size, windowContextSettings settings)
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
