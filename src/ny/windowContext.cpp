#include <ny/windowContext.hpp>
#include <ny/drawContext.hpp>
#include <ny/window.hpp>
#include <ny/app.hpp>

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
    nyMainApp()->sendEvent(std::make_unique<drawEvent>(&window_));
}

//virtual////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
virtualWindowContext::virtualWindowContext(childWindow& win, const windowContextSettings& s) : windowContext(win, s)
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

drawContext& virtualWindowContext::beginDraw(drawContext& dc)
{
    drawContext_ = new redirectDrawContext(dc, window_.getPosition(), window_.getSize());
    drawContext_->startClip();
    return *drawContext_;
};

void virtualWindowContext::finishDraw()
{
    if(drawContext_)
    {
        drawContext_->apply(); //needed here?
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
