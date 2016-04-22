#include <ny/gui/button.hpp>
#include <ny/draw/drawContext.hpp>
#include <ny/app/mouse.hpp>

namespace ny
{

//Drawer
void Button::DefaultDrawer::draw(Button& b, DrawContext& dc)
{
	Color background(0x00000099);
	if(b.pressed())
	{
		background.rgbaInt(0xCCCCCC66);
	}
	else if(b.mouseOver())
	{
		background.rgbaInt(0xCCCCCC33);
	}

	Rectangle rct({0, 0}, b.size());
	rct.borderRadius(4);
	dc.draw({rct, background, Pen::none});

	unsigned int labelSize = 18;

	Text label(b.size() / 2, b.label(), labelSize);
	label.horzBounds(Text::HorzBounds::center);
	label.vertBounds(Text::VertBounds::middle);

	dc.draw({label, Color(255, 255, 255, 200), Pen::none});
}

//Button
bool Button::handleEvent(const Event& event)
{
	if(Widget::handleEvent(event)) return 1;

	if(event.type() == eventType::mouseButton)
	{
		mouseButtonEvent(static_cast<const MouseButtonEvent&>(event));
		return 1;
	}

	return 0;
}

void Button::mouseButtonEvent(const MouseButtonEvent& event)
{
	if(event.button == Mouse::Button::left)
	{
		if(event.pressed)
		{
			pressed_ = 1;
			requestRedraw();
		}
		else
		{
			if(mouseOver_)
			{
				onClick(*this);
			}

			pressed_ = 0;
			requestRedraw();
		}
	}
}

void Button::mouseCrossEvent(const MouseCrossEvent& event)
{
	Widget::mouseCrossEvent(event);
	requestRedraw();
}

}
