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
	std::unique_ptr<NativeWidget> nativeWidget_ {nullptr};

public:
	Widget(WidgetBase& parent);
	~Widget();

	virtual bool handleEvent(const Event& event) override;

	virtual rect2ui extents() const;
	virtual void draw(DrawContext& dc);
	virtual bool contains(const vec2ui& point) const { return nytl::contains(extents(), point); }

	virtual std::string widgetClass() const { return "ny::Widget"; }
	NativeWidget* nativeWidget() const { return nativeWidget_.get(); }
};

}
