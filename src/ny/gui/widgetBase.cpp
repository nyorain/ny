#include <ny/gui/widgetBase.hpp>
#include <ny/gui/widget.hpp>
#include <ny/gui/gui.hpp>

#include <ny/draw/drawContext.hpp>
#include <ny/app/mouse.hpp>
#include <ny/app/keyboard.hpp>
#include <ny/window/events.hpp>

#include <stdexcept>

namespace ny
{

bool WidgetBase::handleEvent(const Event& event)
{
	switch(event.type())
	{
		case eventType::mouseCross:
			mouseCrossEvent(static_cast<const MouseCrossEvent&>(event));
			break;
		case eventType::windowFocus:
			focusEvent(static_cast<const FocusEvent&>(event));
			break;
		case eventType::windowShow:
			showEvent(static_cast<const ShowEvent&>(event));
			break;
		default:
			return 0;
	}

	return 1;
}

void WidgetBase::mouseCrossEvent(const MouseCrossEvent& event)
{
	if(event.entered)
	{
		mouseOver_ = 1;
	}
	else
	{
		mouseOver_ = 0;
	}
}

void WidgetBase::focusEvent(const FocusEvent& event)
{
	if(event.gained)
	{
		focus_ = 1;
	}
	else
	{
		focus_ = 0;
	}
}

void WidgetBase::showEvent(const ShowEvent& event)
{
	if(event.show)
	{
		shown_ = 1;
	}
	else
	{
		shown_ = 0;
	}
}

void WidgetBase::show()
{
	shown_ = 1;
}

void WidgetBase::hide()
{
	shown_ = 0;
}

Widget* WidgetBase::widget(const Vec2i& position)
{
	for(auto& child : children())
	{
		if(contains(child->extents(), position))
		{
			return child->widget(position);
		}
	}

	return nullptr;
}

const Widget* WidgetBase::widget(const Vec2i& position) const
{
	for(auto& child : children())
	{
		if(contains(child->extents(), position))
		{
			return child->widget(position);
		}
	}

	return nullptr;
}

Vec2i WidgetBase::relativePosition(const Widget& widgt) const
{
	auto ret = Vec2i{0, 0};
	const Widget* wit = &widgt; //horizontal widget iterator

	while(wit != this)
	{
		ret += wit->position();

		auto& parnt = wit->parent();
		if(&parnt.root() == &parnt && &parnt != this) //is root, not this -> invalid parameter
		{
			if(&parnt != this)
			{
				throw std::invalid_argument("Widget::relativePosition: "
					"widget parameter is not child");
			}
			else
			{
				break; //do not reinterpret_cast it!
			}
		}

		wit = reinterpret_cast<const Widget*>(&parnt); //Must be widget type since it isnt root
	}

	return ret;
}

void WidgetBase::draw(DrawContext& dc)
{
	for(auto& child : children())
	{
		ny::RedirectDrawContext rdc(dc, child->position(), child->size());
		child->draw(rdc);
		// child->draw(dc);
	}
}

}
