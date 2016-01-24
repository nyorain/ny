#pragma once

#include <ny/include.hpp>
#include <ny/gui/widgetBase.hpp>

#include <nytl/callback.hpp>
#include <nytl/hierachy.hpp>

namespace ny
{

class Gui : public hierachyRoot<WidgetBase>
{
protected:
	Window* window_;
	std::vector<raiiConnection> connections_;

public:
	Gui() = default; //independent gui
	Gui(Window& window); //windowGui
	virtual ~Gui();

	virtual bool handleEvent(const Event& event) override;
	virtual void draw(DrawContext& dc);

	Window* window() const { return window_; }
};

}
