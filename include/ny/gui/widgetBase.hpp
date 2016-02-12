#pragma once

#include <ny/include.hpp>
#include <ny/app/eventHandler.hpp>

#include <nytl/callback.hpp>
#include <nytl/hierachy.hpp>
#include <nytl/vec.hpp>

namespace ny
{

class WidgetBase : public EventHandler, public hierachyBase<Gui, Widget>
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

	virtual Widget* widget(const Vec2i& position);
	virtual const Widget* widget(const Vec2i& position) const;

	virtual void draw(DrawContext& dc);
	virtual void requestRedraw() = 0;
};

}
