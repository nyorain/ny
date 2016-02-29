#include <ny/gui/widget.hpp>
#include <ny/gui/gui.hpp>
#include <ny/gui/style.hpp>

#include <ny/window/nativeWidget.hpp>

namespace ny
{

Widget::Widget(WidgetBase& parent, const Vec2i& position, const Vec2ui& size) 
	: HierachyNode<WidgetBase>(parent), position_(position), size_(size), nativeWidget_(nullptr)
{
}

Widget::~Widget() = default;

void Widget::mouseCrossEvent(const MouseCrossEvent& event)
{
	WidgetBase::mouseCrossEvent(event);
	onMouseCross(*this, event);
}

void Widget::focusEvent(const FocusEvent& event)
{
	WidgetBase::focusEvent(event);
	onFocus(*this, event);
}

void Widget::showEvent(const ShowEvent& event)
{
	WidgetBase::showEvent(event);
	onShow(*this, event);
}

Widget* Widget::widget(const Vec2i& position)
{
	auto ret = WidgetBase::widget(position);
	if(!ret) return this;
	return ret;
}

const Widget* Widget::widget(const Vec2i& position) const
{
	auto ret = WidgetBase::widget(position);
	if(!ret) return this;
	return ret;
}

void Widget::draw(DrawContext& dc)
{
	WidgetStyle::draw(*this, dc);
	WidgetBase::draw(dc);
}

void Widget::requestRedraw()
{
	root().requestRedraw();
}

}
