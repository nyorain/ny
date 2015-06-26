#include "backends/windowContext.hpp"
#include "graphics/drawContext.hpp"
#include "window/window.hpp"

namespace ny
{

//wc/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
windowContext::windowContext(window& win, unsigned long hints) : window_(win), hints_(hints)
{
}

windowContext::windowContext(window& win, const windowContextSettings& s) : window_(win), hints_(s.hints)
{
}


void windowContext::redraw()
{
    drawEvent e;
    e.handler = &window_;
    window_.processEvent(e);
}


//toplevel/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
toplevelWindowContext::toplevelWindowContext(toplevelWindow& win, const windowContextSettings& s) : windowContext(win, s)
{
}

toplevelWindow& toplevelWindowContext::getToplevelWindow() const
{
    return static_cast<toplevelWindow&>(window_);
}

//child/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
childWindowContext::childWindowContext(childWindow& win, const windowContextSettings& s) : windowContext(win, s)
{
}

childWindow& childWindowContext::getChildWindow() const
{
    return static_cast<childWindow&>(window_);
}

//virtual////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
virtualWindowContext::virtualWindowContext(childWindow& win, const windowContextSettings& s) : windowContext(win, s), childWindowContext(win, s), drawContext_(nullptr)
{
}

void virtualWindowContext::refresh()
{
    window_.getParent()->refresh();
};

drawContext& virtualWindowContext::beginDraw()
{
    drawContext_ = new redirectDrawContext(getParentContext()->beginDraw(), window_.getPosition(), window_.getSize());

    drawContext_->startClip();

    return *drawContext_;
};

void virtualWindowContext::finishDraw()
{
    if(drawContext_)
    {
        drawContext_->apply();
        drawContext_->endClip();
        delete drawContext_;
    }

    drawContext_ = nullptr;

    getParentContext()->finishDraw();
};

void virtualWindowContext::setSize(vec2ui size, bool change)
{
}

void virtualWindowContext::setPosition(vec2i position, bool change)
{
}

windowContext* virtualWindowContext::getParentContext() const
{
    return window_.getParent()->getWindowContext();
}

void virtualWindowContext::updateCursor(mouseCrossEvent* ev)
{
    getParentContext()->setCursor(window_.getCursor());
    getParentContext()->updateCursor(ev);
}

}
