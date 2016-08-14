#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/common/xkb.hpp>
#include <ny/backend/mouseContext.hpp>

namespace ny
{

///X11 MouseContext implementation
class X11MouseContext : public MouseContext
{
public:
	X11MouseContext(X11AppContext& ac) : appContext_(ac) {}

	//MouseContext
	Vec2ui position() const override;
	bool pressed(MouseButton button) const override;
	WindowContext* over() const override { return over_; }

	//specific
	void over(WindowContext* ctx);
	void move(Vec2ui position);

protected:
	X11AppContext& appContext_;
	WindowContext* over_;
};


///X11 KeyboardContext implementation
class X11KeyboardContext : public XkbKeyboardContext
{
public:
	X11KeyboardContext(X11AppContext& ac);
	~X11KeyboardContext();

	//KeyboardContext impl
	bool pressed(Key key) const override;
	WindowContext* focus() const override { return focus_; }

protected:
	X11AppContext& appContext_;
	WindowContext* focus_;
};

}

