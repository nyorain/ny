#pragma once

#include <ny/include.hpp>
#include <ny/gui/widget.hpp>
#include <ny/gui/style.hpp>

namespace ny
{
	
class Textfield : public Widget
{
public:
	struct DefaultDrawer : public WidgetDrawer<Textfield>
	{
		virtual void draw(Textfield& b, DrawContext& dc) override;
	};

protected:
	std::string label_;

public:
	using Widget::Widget;

	std::string label() const { return label_; }
	void label(const std::string lbl){ label_ = lbl; }

	void submit();

	virtual bool handleEvent(const Event& event) override;
	virtual void mouseButtonEvent(const MouseButtonEvent& event);
	virtual void keyEvent(const KeyEvent& event);

	virtual std::string widgetClass() const override { return "ny::Textfield"; }

public:
	callback<void(Textfield&)> onChange;
	callback<void(Textfield&)> onEnter;
};

}
