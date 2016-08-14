#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/mouseContext.hpp>
#include <ny/backend/keyboardContext.hpp>

namespace ny
{

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
};

///Winapi KeyboardContext implementation.
class WinapiKeyboardContext : public KeyboardContext
{
public:
	WinapiKeyboardContext(WinapiAppContext& context) : context_(context) {}

	bool pressed(Key key) const override;
	std::string unicode(Key key) const override;
	WindowContext* focus() const override;

	std::string unicode(unsigned int vkcode) const;

protected:
	WinapiAppContext& context_;
};

}
