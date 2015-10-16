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
    nyMainApp()->sendEvent(make_unique<drawEvent>(&window_));
}

//virtual////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
virtualWindowContext::virtualWindowContext(childWindow& win, const windowContextSettings& s) : windowContext(win, s), drawContext_(nullptr)
{
}

virtualWindowContext::~virtualWindowContext() = default;

void virtualWindowContext::refresh()
{
    window_.getParent()->refresh();
};

drawContext* virtualWindowContext::beginDraw()
{
    auto dc = getParentContext()->beginDraw();
    if(!dc) return nullptr;

    drawContext_.reset(new redirectDrawContext(*dc, window_.getPosition(), window_.getSize()));
    drawContext_->startClip();

    return drawContext_.get();
};

drawContext* virtualWindowContext::beginDraw(drawContext& dc)
{
    drawContext_.reset(new redirectDrawContext(dc, window_.getPosition(), window_.getSize()));
    drawContext_->startClip();

    return drawContext_.get();
};

void virtualWindowContext::finishDraw()
{
    if(drawContext_.get())
    {
        drawContext_->apply(); //needed here?
        drawContext_->endClip();
    }

    drawContext_.reset();
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
    windowContext* ret = nullptr;
    if(!window_.getParent() || !(ret = window_.getParent()->getWindowContext()))
    {
        //error...
    }

    return ret;
}

void virtualWindowContext::updateCursor(const mouseCrossEvent* ev)
{
    getParentContext()->setCursor(window_.getCursor());
    getParentContext()->updateCursor(ev);
}

}
