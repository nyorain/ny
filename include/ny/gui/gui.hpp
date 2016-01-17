#pragma once

#include <ny/include.hpp>
#include <ny/gui/widgetBase.hpp>

#include <nytl/callback.hpp>
#include <nytl/hierachy.hpp>

namespace ny
{

class Gui : public hierachyRoot<WidgetBase>
{
public:
	Gui() = default;
	Gui(Window& window);
	virtual ~Gui();

	virtual bool processEvent(const Event& event) override;
	virtual void draw(DrawContext& dc);
};

}
