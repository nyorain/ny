#include <ny/gui/textfield.hpp>
#include <ny/draw/drawContext.hpp>
#include <ny/app/mouse.hpp>
#include <ny/app/keyboard.hpp>


namespace ny
{

//Drawer
void Textfield::DefaultDrawer::draw(Textfield& tf, DrawContext& dc)
{
	dc.clear(Color::black);
}

//Texfield
bool Textfield::handleEvent(const Event& event)
{
	if(Widget::handleEvent(event)) return 1;

	if(event.type() == eventType::mouseButton)
	{
		mouseButtonEvent(static_cast<const MouseButtonEvent&>(event));
		return 1;
	}
	else if(event.type() == eventType::key)
	{
		keyEvent(static_cast<const KeyEvent&>(event));
		return 1;
	}

	return 0;
}

void Textfield::mouseButtonEvent(const MouseButtonEvent& event)
{
}

void Textfield::keyEvent(const KeyEvent& event)
{
}

void Textfield::submit()
{
}

}
