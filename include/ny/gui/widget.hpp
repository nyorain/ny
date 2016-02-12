#pragma once

#include <ny/include.hpp>
#include <ny/gui/widgetBase.hpp>

#include <nytl/vec.hpp>
#include <nytl/rect.hpp>
#include <nytl/hierachy.hpp>

namespace ny
{

class Widget : public hierachyNode<WidgetBase>
{
protected:
	Vec2i position_ {0, 0};
	Vec2ui size_ {0, 0};

	std::unique_ptr<NativeWidget> nativeWidget_;

public:
	Widget() = default;
	Widget(WidgetBase& parent, const Vec2i& position, const Vec2ui& size);
	~Widget();

	Vec2i position() const { return position_; }
	Vec2ui size() const { return size_; }
	Rect2i extents() const { return {position(), size()}; }

	//virtual bool handleEvent(const Event& event) override;
	virtual void mouseCrossEvent(const MouseCrossEvent& event) override;
	virtual void focusEvent(const FocusEvent& event) override;
	virtual void showEvent(const ShowEvent& event) override;

	virtual void draw(DrawContext& dc) override;
	virtual void requestRedraw() override;

	virtual Widget* widget(const Vec2i& position) override;
	virtual const Widget* widget(const Vec2i& position) const override;

	virtual std::string widgetClass() const { return "ny::Widget"; }
	NativeWidget* nativeWidget() const { return nativeWidget_.get(); }

public:
	Callback<void(Widget&, const MouseCrossEvent&)> onMouseCross;
	Callback<void(Widget&, const FocusEvent&)> onFocus;
	Callback<void(Widget&, const ShowEvent&)> onShow;
};

}
