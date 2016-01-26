#pragma once

#include <ny/include.hpp>
#include <ny/gui/widget.hpp>
#include <ny/gui/style.hpp>

namespace ny
{
	
class Button : public Widget
{
public:
	struct DefaultDrawer : public WidgetDrawer<Button>
	{
		virtual void draw(Button& b, DrawContext& dc) override;
	};

protected:
	bool pressed_ {0};
	std::string label_;

public:
	using Widget::Widget;

	bool pressed() const { return pressed_; }

	std::string label() const { return label_; }
	void label(const std::string lbl){ label_ = lbl; }

	virtual bool handleEvent(const Event& event) override;
	virtual void mouseCrossEvent(const MouseCrossEvent& event) override;
	virtual void mouseButtonEvent(const MouseButtonEvent& event);

	virtual std::string widgetClass() const override { return "ny::Button"; }

public:
	callback<void(Button&)> onClick;
};

}
