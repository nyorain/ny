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
#include <ny/widgets.hpp>
#include <ny/sizer.hpp>

#include <nytl/misc.hpp>

#include <iostream>
#include <climits>

namespace ny
{

//wcSettings
windowContextSettings::~windowContextSettings() = default;

//window//////////////////////////////////////////////////////////////////////////////
window::window(eventHandlerNode& parent, vec2ui position, vec2ui size, const windowContextSettings& settings) : eventHandlerNode(), surface(), maxSize_(UINT_MAX, UINT_MAX)
{
    create(parent, size, position);
}

window::window() = default;
window::~window() = default;

void window::create(eventHandlerNode& parent, vec2i position, vec2ui size, const windowContextSettings& settings)
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
        eventHandlerNode::create(parent);
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
}

void window::destroy()
{
    destroyCallback_(*this);
    eventHandlerNode::destroy();
    windowContext_.reset();
}

bool window::processEvent(const event& ev)
{
    //if(!checkValid()) return 0;
    if(eventHandlerNode::processEvent(ev)) return 1;

    switch (ev.type())
    {
    case eventType::mouseButton:
        mouseButton(event_cast<mouseButtonEvent>(ev));
        return true;
    case eventType::mouseMove:
        mouseMove(event_cast<mouseMoveEvent>(ev));
        return true;
    case eventType::mouseCross:
        mouseCross(event_cast<mouseCrossEvent>(ev));
        return true;
    case eventType::mouseWheel:
        mouseWheel(event_cast<mouseWheelEvent>(ev));
        return true;
    case eventType::key:
        keyboardKey(event_cast<keyEvent>(ev));
        return true;
    case eventType::windowFocus:
        windowFocus(event_cast<focusEvent>(ev));
        return true;
    case eventType::windowSize:
        windowSize(event_cast<sizeEvent>(ev));
        return true;
    case eventType::windowPosition:
        windowPosition(event_cast<positionEvent>(ev));
        return true;
    case eventType::windowDraw:
        windowDraw(event_cast<drawEvent>(ev));
        return true;
    case eventType::windowShow:
        windowShow(event_cast<showEvent>(ev));
        return true;
    case eventType::windowRefresh:
        refresh();
        return true;
    case eventType::context:
        windowContext_->processEvent(event_cast<contextEvent>(ev));
        return true;

    default:
        return false;
    }
}

bool window::valid() const
{
    return eventHandlerNode::valid() && windowContext_.get();
}


//////////////////////////////////////////////
bool window::checkValid() const
{
    if(valid()) return 1;

    nyWarning("Used invalid window ", this, ", action will not be executed");
    return 0;
}

void window::refresh()
{
    if(!checkValid()) return;
    windowContext_->refresh();
}

void window::setSize(vec2ui size)
{
    if(!checkValid()) return;

    size_ = size;
    windowContext_->setSize(size, 1);

    resizeCallback_(*this, size_);
}

void window::setPosition(vec2i position)
{
    if(!checkValid()) return;

    position_ = position;
    windowContext_->setPosition(position_);

    moveCallback_(*this, position_);
}

void window::move(vec2i delta)
{
    if(!checkValid()) return;

    position_ += delta;
    windowContext_->setPosition(position_);

    moveCallback_(*this, position_);
}

void window::show()
{
    if(!checkValid()) return;

    windowContext_->show();
    shown_ = 1;
}

void window::hide()
{
    if(!checkValid()) return;

    windowContext_->hide();
    shown_ = false;
}

void window::toggleShow()
{
    if(!checkValid()) return;

    if(isShown())
        windowContext_->hide();

    else
        windowContext_->show();
}

void window::setMaxSize(vec2ui size)
{
    if(!checkValid()) return;

    maxSize_ = size;
    windowContext_->setMaxSize(size);
}

void window::setMinSize(vec2ui size)
{
    if(!checkValid()) return;

    minSize_ = size;
    windowContext_->setMinSize(size);
}

//event callbacks
void window::mouseMove(const mouseMoveEvent& e)
{
    mouseMoveCallback_(*this, e);
}
void window::mouseCross(const mouseCrossEvent& e)
{
    mouseOver_ = e.entered;
    mouseCrossCallback_(*this, e);
}
void window::mouseButton(const mouseButtonEvent& e)
{
    mouseButtonCallback_(*this, e);
}
void window::mouseWheel(const mouseWheelEvent& e)
{
    mouseWheelCallback_(*this, e);
}
void window::keyboardKey(const keyEvent& e)
{
    keyCallback_(*this, e);
}
void window::windowSize(const sizeEvent& e)
{
    size_ = e.size;
    windowContext_->setSize(size_, e.change);
    resizeCallback_.call(*this, size_);
}
void window::windowPosition(const positionEvent& e)
{
    position_ = e.position;
    windowContext_->setPosition(position_, e.change);
    moveCallback_(*this, position_);
}
void window::windowDraw(const drawEvent& e)
{
    drawContext* dc = windowContext_->beginDraw();
    draw(*dc);
    windowContext_->finishDraw();
}
void window::windowShow(const showEvent& e)
{
    showCallback_(*this, e);
}
void window::windowFocus(const focusEvent& e)
{
    focus_ = e.focusGained;
    focusCallback_(*this, e);
}

//draw
void window::draw(drawContext& dc)
{
    dc.clear(color::white); //todo
    drawCallback_(*this, dc);

    for(auto win : getWindowChildren())
        if(win->isVirtual()) win->draw(*((virtualWindowContext*) win->getWindowContext())->beginDraw(dc));
}

//util
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
    if(!checkValid()) return;
    windowContext_->setCursor(curs);
}

void window::addWindowHints(unsigned long hints)
{
    hints &= ~hints_; //remove all the hints, which are still set to make it easier for windowContext
    hints_ |= hints;

    if(!checkValid()) return;

    windowContext_->addWindowHints(hints);
}

void window::removeWindowHints(unsigned int hints)
{
    hints &= hints_; //remove all the hints, which are not set and cant be removed to make it easier for windowContext
    hints_ &= ~hints;

    if(!checkValid()) return;

    windowContext_->removeWindowHints(hints);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//toplevelwindow///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
toplevelWindow::toplevelWindow() : window()
{
}

toplevelWindow::toplevelWindow(vec2i position, vec2ui size, std::string title, const windowContextSettings& settings) : window(), headerbar_(nullptr), panel_(nullptr)
{
    create(position, size, title, settings);
}

toplevelWindow::~toplevelWindow() = default;

void toplevelWindow::create(vec2i position, vec2ui size, std::string title, const windowContextSettings& settings)
{
    title_ = title;
/*
    hints_ |= windowHints::Toplevel;
    hints_ |= windowHints::Resize;
	hints_ |= windowHints::Move;

	hints_ |= windowHints::CustomDecorated; //...
	hints_ |= windowHints::CustomMoved; //...
	hints_ |= windowHints::CustomResized; //...
*/
    window::create(*nyMainApp(), position, size, settings);

    cursor c;
    c.fromNativeType(cursorType::leftPtr);
    windowContext_->setCursor(c);
/*
    if(hints_ & windowHints::CustomDecorated)
    {
        unsigned int hheight = 100;
        unsigned int padding = 15;
        unsigned int border = 10;

        setHeight(getHeight() + hheight + 2 * border);
        setWidth(getWidth() + 2 * border);

        headerbar_.reset(new headerbar(*this, vec2i(0, 0), vec2ui(getWidth() - 2 * border, hheight - 2 * border)));
        headerbar_->refresh();

        vboxSizer* b = new vboxSizer(*this);

        hboxSizer* b1 = new hboxSizer(*b);
        b1->addChild(*headerbar_);

        panel_.reset(new panel(*this, vec2i(padding, hheight + padding), vec2ui(getWidth() - 2 * (border + padding), getHeight() - hheight - 2 * (border + padding))));
        panel_->refresh();

        hboxSizer* b2 = new hboxSizer(*b);
        b2->addChild(*panel_);
    }
*/
}
/*
void toplevelWindow::addChild(eventHandler& child)
{
    auto wid = dynamic_cast<widget*>(&child);
    bool dont = 0;
    if(wid)
    {
        if(wid->getWidgetName() == "ny::headerbar" || wid->getWidgetName() == "ny::panel")
            dont = 1;
    }

    if(panel_.get() && !dont) child.processEvent(reparentEvent(&child, panel_.get()));
    else
        eventHandler::addChild(child);
}
*/
void toplevelWindow::draw(drawContext& dc)
{
/*
    if(hints_ & windowHints::CustomDecorated)
    {
        dc.resetClip();
        dc.clear(color(0, 0, 0, 100));
        redirectDrawContext red(dc, vec2i(10, 10), getSize() - vec2ui(20, 20));
        red.startClip();
        window::draw(dc);
        red.endClip();
    }
    else
    {
        window::draw(dc);
    }
*/
    window::draw(dc);
}

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
*/

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
*/
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
*/
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

}
