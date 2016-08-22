#include <ny/backend/x11/input.hpp>
#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/windowContext.hpp>

#include <xcb/xcb.h>

namespace ny
{

//Mouse
nytl::Vec2ui X11MouseContext::position() const
{
	if(!over_) return {};
	auto cookie = xcb_query_pointer(appContext_.xConnection(), over_->xWindow());
	auto reply = xcb_query_pointer_reply(appContext_.xConnection(), cookie, nullptr);

	return nytl::Vec2ui(reply->win_x, reply->win_y);
}

bool X11MouseContext::pressed(MouseButton button) const
{
	return buttonStates_[static_cast<unsigned char>(button)];
}

WindowContext* X11MouseContext::over() const
{
	return over_;
}

//Keyboard
bool X11KeyboardContext::pressed(Key key) const
{
	return keyStates_[static_cast<unsigned char>(key)];
}
	
}
