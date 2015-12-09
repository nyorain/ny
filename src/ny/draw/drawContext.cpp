#include <ny/draw/drawContext.hpp>
#include <ny/draw/shape.hpp>

#include <nytl/log.hpp>

namespace ny
{

void DrawContext::mask(const PathBase& obj)
{
    switch(obj.type())
    {
		case PathBase::Type::text: mask(obj.text()); return;
		case PathBase::Type::rectangle: mask(obj.rectangle()); return;
		case PathBase::Type::path: mask(obj.path()); return;
		case PathBase::Type::circle: mask(obj.circle()); return;
    }
}

void DrawContext::mask(const std::vector<PathBase>& m)
{
    for(auto& pth : m)
        mask(pth);
}

void DrawContext::mask(const Rectangle& obj)
{
    mask(obj.asPath());
}

void DrawContext::mask(const Circle& obj)
{
    mask(obj.asPath());
}

void DrawContext::draw(const Shape& obj)
{
    mask(obj.pathBase());

    fillPreserve(obj.brush());
    strokePreserve(obj.pen());

    resetMask();
}

void DrawContext::clear(const Brush& b)
{
    Brush alphaMask(Color(0, 0, 0, 255));
    paint(alphaMask, b);
}

void DrawContext::fill(const Brush& col)
{
    fillPreserve(col);
    resetMask();
}

void DrawContext::stroke(const Pen& col)
{
    strokePreserve(col);
    resetMask();
}

void DrawContext::clipMask()
{
    sendWarning("DrawContext::clipMak: mask clipping not supported, object ", this);
}

void DrawContext::clipMaskPreserve()
{
    sendWarning("DrawContext::clipMakPreserve: mask clipping not supported, object ", this);
}

std::vector<PathBase> DrawContext::maskClip() const
{
    sendWarning("DrawContext::maskClip: mask clipping not supported, object ", this);
    return {};
}

void DrawContext::resetMaskClip()
{
    sendWarning("DrawContext::resetMaskClip: mask clipping not supported, object ", this);
}

//redirectDrawContext//////////////////////////////////////////////////////////////////////////////////////////////////////
RedirectDrawContext::RedirectDrawContext(DrawContext& redirect, const vec2f& position, 
		const vec2f& size)
    : DrawContext(), size_(size), position_(position), redirect_(&redirect)
{
}


void RedirectDrawContext::apply()
{
    redirect_->apply();
}

void RedirectDrawContext::clear(const Brush& b)
{
    //todo: transklate brushes correctly
    redirect_->clear(b);
}

void RedirectDrawContext::paint(const Brush& alphaMask, const Brush& fillBrush)
{
    //todo: transklate brushes correctly
    redirect_->paint(alphaMask, fillBrush);
}

void RedirectDrawContext::mask(const Path& obj)
{
    auto scopy = obj;
    scopy.move(position_);
    redirect_->mask(scopy);
}

void RedirectDrawContext::mask(const Rectangle& obj)
{
    auto scopy = obj;
    scopy.move(position_);
    redirect_->mask(scopy);
}

void RedirectDrawContext::mask(const Text& obj)
{
    auto scopy = obj;
    scopy.move(position_);
    redirect_->mask(scopy);
}

void RedirectDrawContext::mask(const Circle& obj)
{
    auto scopy = obj;
    scopy.move(position_);
    redirect_->mask(scopy);
}

void RedirectDrawContext::mask(const PathBase& obj)
{
    auto scopy = obj;
    scopy.move(position_);
    redirect_->mask(scopy);
}

void RedirectDrawContext::resetMask()
{
    redirect_->resetMask();
}

void RedirectDrawContext::fillPreserve(const Brush& col)
{
    redirect_->fillPreserve(col);
}
void RedirectDrawContext::strokePreserve(const Pen& col)
{
    redirect_->strokePreserve(col);
}

void RedirectDrawContext::fill(const Brush& col)
{
    redirect_->fill(col);
}
void RedirectDrawContext::stroke(const Pen& col)
{
    redirect_->stroke(col);
}

bool RedirectDrawContext::maskClippingSupported() const
{
    return redirect_->maskClippingSupported();
}

void RedirectDrawContext::clipMask()
{
    redirect_->clipMask();
}

void RedirectDrawContext::clipMaskPreserve() 
{
    redirect_->clipMaskPreserve();
}

std::vector<PathBase> RedirectDrawContext::maskClip() const
{
    auto ret = redirect_->maskClip();
    for(auto& p : ret)
        p.move(-position_);

    return ret;
}

void RedirectDrawContext::resetMaskClip()
{
    redirect_->resetMaskClip();
}

void RedirectDrawContext::size(const vec2f& size)
{
    size_ = size;
}
void RedirectDrawContext::position(const vec2f& position)
{
    position_ = position;
}

void RedirectDrawContext::redirect(DrawContext& dc)
{
    redirect_= &dc;
}

void RedirectDrawContext::clipRectangle(const rect2f& obj)
{
    rect2f clipRect;
    clipRect.position = max(obj.position, {0.f, 0.f}) + position_;
    clipRect.size = min(obj.size, position_ + size_ - clipRect.position);

	redirect_->clipRectangle(clipRect);
};

void RedirectDrawContext::resetRectangleClip()
{
    redirect_->clipRectangle(extents());
}

rect2f RedirectDrawContext::rectangleClip() const
{
    auto r = redirect_->rectangleClip();
    r.position -= position_;

    return r;
}

void RedirectDrawContext::startDrawing()
{
    redirect_->resetMask();

    if(redirect_->maskClippingSupported())
    {
        maskClipSave_ = redirect_->maskClip();
        redirect_->resetMaskClip();
    }

    rectangleClipSave_ = redirect_->rectangleClip();
    redirect_->clipRectangle(extents());
}


void RedirectDrawContext::endDrawing()
{
    redirect_->resetMask();

    if(redirect_->maskClippingSupported())
    {
        redirect_->resetMaskClip();
        redirect_->mask(maskClipSave_);
        redirect_->clipMask();
    }

    redirect_->clipRectangle(rectangleClipSave_);
}

//Delayed
void DelayedDrawContext::mask(const Path& obj)
{
	mask_.push_back(obj);
}

void DelayedDrawContext::mask(const Rectangle& obj)
{
	mask_.push_back(obj);
}

void DelayedDrawContext::mask(const Text& obj)
{
	mask_.push_back(obj);
}

void DelayedDrawContext::mask(const Circle& obj)
{
	mask_.push_back(obj);
}

void DelayedDrawContext::mask(const PathBase& obj)
{
	mask_.push_back(obj);
}

void DelayedDrawContext::resetMask()
{
	mask_.clear();
}

}
