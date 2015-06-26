#include <ny/graphics/drawContext.hpp>
#include <ny/graphics/shape.hpp>
#include <ny/app/surface.hpp>

namespace ny
{

drawContext::drawContext(surface& s) : surface_(s)
{
}

drawContext::~drawContext()
{
}

void drawContext::mask(const ny::mask& m)
{
    auto vec = m.paths();

    for(size_t i(0); i < vec.size(); i++)
        mask(vec[i]);
}

void drawContext::draw(const shape& obj)
{
    mask(obj.getMask());
    fill(obj.getBrush());
    outline(obj.getPen());
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

void redirectDrawContext::mask(const path& obj)
{
    path scopy = obj;
    scopy.translate(position_);
    redirect_.mask(scopy);
}

void redirectDrawContext::resetMask()
{
    redirect_.resetMask();
}

void redirectDrawContext::fill(const brush& col)
{
    redirect_.fill(col);
}
void redirectDrawContext::outline(const pen& col)
{
    redirect_.outline(col);
}

void redirectDrawContext::setSize(vec2d size)
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
