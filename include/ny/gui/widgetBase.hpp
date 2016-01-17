#pragma once

#include <ny/include.hpp>
#include <ny/app/eventHandler.hpp>

#include <nytl/callback.hpp>
#include <nytl/hierachy.hpp>

namespace ny
{

class WidgetBase : public EventHandler, public hierachyBase<WidgetBase>
{
protected:
	bool focus_ {0};
	bool mouseOver_ {0};
	bool shown_ {1};

protected:
	virtual void mouseCrossEvent(const MouseCrossEvent& event);
	virtual void focusEvent(const FocusEvent& event);
	virtual void showEvent(const ShowEvent& event);

public:
	bool focus() const { return focus_; }
	bool mouseOver() const { return mouseOver_; }
	bool shown() const { return shown_; }

	void show();
	void hide();

	virtual bool processEvent(const Event& event) override;

public:
	callback<void(MouseCrossEvent&)> onMouseCross;
	callback<void(FocusEvent&)> onFocus;
	callback<void(ShowEvent&)> onShow;
};

}
