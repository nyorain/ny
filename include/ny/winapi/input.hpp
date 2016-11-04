#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/winapi/windowContext.hpp>
#include <ny/backend/mouseContext.hpp>
#include <ny/backend/keyboardContext.hpp>

namespace ny
{

//TODO: set over and focus (from AppContext)
//TODO: call callbacks correctly (trigger events from AppContext)
///Winapi MouseContext implementation.
class WinapiMouseContext : public MouseContext
{
public:
	WinapiMouseContext(WinapiAppContext& context) : context_(context) {}

	Vec2ui position() const override;
	bool pressed(MouseButton button) const override;
	WinapiWindowContext* over() const override { return over_; }

	//winapi specific
	void over(WinapiWindowContext* wc);
	nytl::Vec2i move(nytl::Vec2i pos); //returns the movement delta

protected:
	WinapiAppContext& context_;
	WinapiWindowContext* over_ {};
	nytl::Vec2i position_;
};

///Winapi KeyboardContext implementation.
class WinapiKeyboardContext : public KeyboardContext
{
public:
	WinapiKeyboardContext(WinapiAppContext& context) : context_(context) {}

	bool pressed(Keycode) const override;
	std::string utf8(Keycode, bool currentState = false) const override;
	WinapiWindowContext* focus() const override { return focus_; }

	//winapi specific
	void focus(WinapiWindowContext* wc);

protected:
	WinapiAppContext& context_;
	WinapiWindowContext* focus_ {};
};

}
