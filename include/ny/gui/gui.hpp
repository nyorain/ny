#pragma once

#include <ny/include.hpp>
#include <ny/gui/widgetBase.hpp>

#include <nytl/callback.hpp>
#include <nytl/hierachy.hpp>

namespace ny
{

class Gui : public HierachyRoot<WidgetBase>
{
protected:
	Window* window_ {nullptr};
	std::vector<RaiiConnection> connections_;

	Widget* focusWidget_ {nullptr};
	Widget* mouseFocusWidget_ {nullptr};
	Widget* mouseOverWidget_ {nullptr};

public:
	Gui() = default; //independent gui
	Gui(Window& window); //windowGui
	virtual ~Gui();

	virtual bool handleEvent(const Event& event) override;
	virtual void mouseCrossEvent(const MouseCrossEvent& event) override;
	virtual void mouseMoveEvent(const MouseMoveEvent& event);
	virtual void mouseButtonEvent(const MouseButtonEvent& event);
	virtual void keyEvent(const KeyEvent& event);

	virtual void draw(DrawContext& dc) override;
	virtual void requestRedraw() override;

	Window* window() const { return window_; }
};

/*
class GuiWindow : public virtual Gui, public virtual Window
{
};
*/

}
