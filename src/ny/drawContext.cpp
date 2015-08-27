#include <ny/drawContext.hpp>
#include <ny/shape.hpp>
#include <ny/surface.hpp>

namespace ny
{

drawContext::drawContext(surface& s) : surface_(s)
{

}

drawContext::~drawContext()
{
}

void drawContext::mask(const path& obj)
{
    switch(obj.getPathType())
    {
        case pathType::text: mask(obj.getText()); return;
        case pathType::rectangle: mask(obj.getRectangle()); return;
        case pathType::custom: mask(obj.getCustom()); return;
        case pathType::circle: mask(obj.getCircle()); return;
    }
}

void drawContext::mask(const ny::mask& m)
{
    for(auto& pth : m)
        mask(pth);
}

void drawContext::mask(const rectangle& obj)
{
    mask(obj.getAsCustomPath());
}

void drawContext::mask(const circle& obj)
{
    mask(obj.getAsCustomPath());
}

void drawContext::draw(const shape& obj)
{
    if(obj.getMask().empty() || (!obj.getPen() && !obj.getBrush()))
        return; //no result

    mask(obj.getMask());

    if(obj.getBrush()) fillPreserve(*obj.getBrush());
    if(obj.getPen()) strokePreserve(*obj.getPen());

    resetMask();
}

void drawContext::clear(color col)
{
    rectangle r(vec2f(0,0), surface_.getSize());
    mask(r);
    fill(col);
    resetMask();
}

void drawContext::fill(const brush& col)
{
    fillPreserve(col);
    resetMask();
}

void drawContext::stroke(const pen& col)
{
    strokePreserve(col);
    resetMask();
}

//redirectDrawContext//////////////////////////////////////////////////////////////////////////////////////////////////////
redirectDrawContext::redirectDrawContext(drawContext& redirect, vec2f position, vec2f size) :
    drawContext(redirect.getSurface()), size_(size), position_(position), redirect_(redirect) {}

redirectDrawContext::redirectDrawContext(drawContext& redirect, vec2f position) :
    drawContext(redirect.getSurface()), size_(redirect.getSurface().getSize()), position_(position), redirect_(redirect) {}

void redirectDrawContext::apply()
{
    redirect_.apply();
}

void redirectDrawContext::clear(color col)
{
    redirect_.clear(col);
}

void redirectDrawContext::mask(const customPath& obj)
{
    customPath scopy = obj;
    scopy.move(position_);
    redirect_.mask(scopy);
}

void redirectDrawContext::mask(const text& obj)
{
    text scopy = obj;
    scopy.move(position_);
    redirect_.mask(scopy);
}

void redirectDrawContext::resetMask()
{
    redirect_.resetMask();
}

void redirectDrawContext::fillPreserve(const brush& col)
{
    redirect_.fillPreserve(col);
}
void redirectDrawContext::strokePreserve(const pen& col)
{
    redirect_.strokePreserve(col);
}

void redirectDrawContext::setSize(vec2f size)
{
    size_ = size;
}
void redirectDrawContext::setPosition(vec2d position)
{
    position_ = position;
}

void redirectDrawContext::startClip()
{
    clipSave_ = redirect_.getClip();
    updateClip();
}

void redirectDrawContext::updateClip()
{
    clipSave_ = redirect_.getClip();
    redirect_.resetClip();
    redirect_.clip(rect2f(position_, size_));
}
void redirectDrawContext::endClip()
{
    redirect_.resetClip();
    redirect_.clip(clipSave_);
}


}
