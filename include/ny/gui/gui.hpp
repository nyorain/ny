#pragma once

#include <ny/include.hpp>
#include <ny/app/eventHandler.hpp>

namespace ny
{

class Gui : public EventHandlerRoot
{
public:
	Gui() = default;
	Gui(Window& window);
	virtual ~Gui() = default;

	virtual bool processEvent(const Event& event) override;

	virtual void draw(DrawContext& dc);
	virtual bool hasMouseOver() const;
	virtual bool hasFocus() const;
};

}
