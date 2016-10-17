#pragma once

#include <ny/backend/winapi/include.hpp>
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
	WindowContext* over() const override;

protected:
	WinapiAppContext& context_;
	WinapiWindowContext* over_ {};
};

///Winapi KeyboardContext implementation.
class WinapiKeyboardContext : public KeyboardContext
{
public:
	WinapiKeyboardContext(WinapiAppContext& context) : context_(context) {}

	bool pressed(Keycode) const override;
	std::string utf8(Keycode, bool currentState = false) const override;
	WindowContext* focus() const override;

protected:
	WinapiAppContext& context_;
	WinapiWindowContext* focus_ {};
};

}
