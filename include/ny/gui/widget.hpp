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
	vec2i position_ {0, 0};
	vec2ui size_ {0, 0};

	std::unique_ptr<NativeWidget> nativeWidget_;

public:
	Widget() = default;
	Widget(WidgetBase& parent, const vec2i& position, const vec2ui& size);
	~Widget();

	vec2i position() const { return position_; }
	vec2ui size() const { return size_; }
	rect2i extents() const { return {position(), size()}; }

	//virtual bool handleEvent(const Event& event) override;
	virtual void mouseCrossEvent(const MouseCrossEvent& event) override;
	virtual void focusEvent(const FocusEvent& event) override;
	virtual void showEvent(const ShowEvent& event) override;

	virtual void draw(DrawContext& dc) override;
	virtual void requestRedraw() override;

	virtual Widget* widget(const vec2i& position) override;
	virtual const Widget* widget(const vec2i& position) const override;

	virtual std::string widgetClass() const { return "ny::Widget"; }
	NativeWidget* nativeWidget() const { return nativeWidget_.get(); }

public:
	callback<void(Widget&, const MouseCrossEvent&)> onMouseCross;
	callback<void(Widget&, const FocusEvent&)> onFocus;
	callback<void(Widget&, const ShowEvent&)> onShow;
};

}
