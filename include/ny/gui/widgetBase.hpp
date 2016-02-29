#pragma once

#include <ny/include.hpp>
#include <ny/base/eventHandler.hpp>

#include <nytl/callback.hpp>
#include <nytl/hierachy.hpp>
#include <nytl/vec.hpp>

namespace ny
{

class WidgetBase : public EventHandler, public HierachyBase<Gui, Widget>
{
protected:
	bool focus_ {0};
	bool mouseOver_ {0};
	bool shown_ {1};

public:
	virtual bool handleEvent(const Event& event) override;
	virtual void mouseCrossEvent(const MouseCrossEvent& event);
	virtual void focusEvent(const FocusEvent& event);
	virtual void showEvent(const ShowEvent& event);

	bool focus() const { return focus_; }
	bool mouseOver() const { return mouseOver_; }
	bool shown() const { return shown_; }

	void show();
	void hide();

	///Returns a relative position of a child widget.
	///\exception std::invalid_argument if the widget parameter is not a child.
	Vec2i relativePosition(const Widget& widget) const;

	///Returns the widget at the given position.
	///If position lays inside the widget but there is no child widget, it returns this widget.
	///If the position does not lay inside the widgets area it returns nullptr.
	virtual Widget* widget(const Vec2i& position);
	virtual const Widget* widget(const Vec2i& position) const;

	///Draws its contents on a given DrawContext.
	virtual void draw(DrawContext& dc);

	///Requests a redraw on whatever the widget is contained.
	virtual void requestRedraw() = 0;
};

}
