#include <ny/gui/gui.hpp>
#include <ny/gui/widget.hpp>

#include <ny/window/window.hpp>
#include <ny/draw/drawContext.hpp>
#include <ny/app/keyboard.hpp>
#include <ny/app/mouse.hpp>
#include <ny/base/log.hpp>

namespace ny
{

Gui::Gui(Window& window) : window_(&window)
{
	connections_.emplace_back(window.onDraw.add({&Gui::draw, this}));
	connections_.emplace_back(window.onMouseCross.add({&Gui::mouseCrossEvent, this}));
	connections_.emplace_back(window.onKey.add({&Gui::keyEvent, this}));
	connections_.emplace_back(window.onMouseButton.add({&Gui::mouseButtonEvent, this}));
	connections_.emplace_back(window.onMouseMove.add({&Gui::mouseMoveEvent, this}));
}

Gui::~Gui()
{
}

bool Gui::handleEvent(const Event& event)
{
	if(WidgetBase::handleEvent(event)) return 1;
	return 0;
}

void Gui::mouseCrossEvent(const MouseCrossEvent& event)
{
	WidgetBase::mouseCrossEvent(event);

	if(mouseOverWidget_)
	{
		MouseCrossEvent crossEvent;
		crossEvent.handler = mouseOverWidget_;
		crossEvent.entered = event.entered;
		crossEvent.position = event.position - relativePosition(*mouseOverWidget_);
		mouseOverWidget_->mouseCrossEvent(crossEvent);
	}
}

void Gui::mouseMoveEvent(const MouseMoveEvent& event)
{
	auto* widgt = widget(event.position);

	if(widgt != mouseOverWidget_)
	{
		MouseCrossEvent crossEvent;
		if(mouseOverWidget_)
		{
			crossEvent.handler = mouseOverWidget_;
			crossEvent.entered = 0;
			crossEvent.position = event.position - relativePosition(*mouseOverWidget_);
			mouseOverWidget_->mouseCrossEvent(crossEvent);
		}
		if(widgt)
		{
			crossEvent.handler = widgt;
			crossEvent.entered = 1;
			crossEvent.position = event.position - relativePosition(*widgt);
			widgt->mouseCrossEvent(crossEvent);
		}

		mouseOverWidget_ = widgt;
	}

	if(mouseOverWidget_)
	{
		auto cpy = event;
		cpy.position = cpy.position - relativePosition(*mouseOverWidget_);
		mouseOverWidget_->handleEvent(cpy);
	}
}

void Gui::mouseButtonEvent(const MouseButtonEvent& event)
{
	if(mouseFocusWidget_)
	{
		auto cpy = event;
		cpy.position = cpy.position - relativePosition(*mouseFocusWidget_);
		mouseFocusWidget_->handleEvent(cpy);

		if(!event.pressed)
		{
			mouseFocusWidget_ = nullptr;
		}		
	}
	else if(mouseOverWidget_)
	{
		auto cpy = event;
		cpy.position = cpy.position - relativePosition(*mouseOverWidget_);
		mouseOverWidget_->handleEvent(cpy);

		if(event.pressed)
		{
			mouseFocusWidget_ = mouseOverWidget_;
		}
	}
}

void Gui::keyEvent(const KeyEvent& event)
{
	if(focusWidget_)
	{
		mouseOverWidget_->handleEvent(event);
	}
}

void Gui::draw(DrawContext& dc)
{
	sendDebug("guidraw");
	dc.clear(Color(0x303030ff));
	WidgetBase::draw(dc);
}

void Gui::requestRedraw()
{
	if(window())
	{
		window()->refresh();
	}
}

}
