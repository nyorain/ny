#pragma once

#include <ny/winapi/include.hpp>
#include <ny/winapi/windowContext.hpp>
#include <ny/mouseContext.hpp>
#include <ny/keyboardContext.hpp>
#include <map>

namespace ny
{

///Winapi MouseContext implementation.
class WinapiMouseContext : public MouseContext
{
public:
	WinapiMouseContext(WinapiAppContext& context) : context_(context) {}
	WinapiMouseContext() = default;

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
	WinapiKeyboardContext(WinapiAppContext& context);
	WinapiKeyboardContext() = default;

	bool pressed(Keycode) const override;
	std::string utf8(Keycode) const override;
	WinapiWindowContext* focus() const override { return focus_; }

	//winapi specific
	void focus(WinapiWindowContext* wc);
	void keyEvent(WinapiWindowContext* wc, unsigned int vkcode, unsigned int lparam);

protected:
	WinapiAppContext& context_;
	WinapiWindowContext* focus_ {};
	std::map<Keycode, std::string> keycodeUnicodeMap_;
};

}
