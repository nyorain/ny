#pragma once

#include <ny/include.hpp>
#include <ny/backend/mouseContext.hpp>

#include <nytl/callback.hpp>

namespace ny
{


///Represents a physical pointer device.
///A Mouse object can be retrieved by an AppContext (or by an App).
class Mouse
{
public:
	Mouse(App& app, MouseContext& mouseContext);
	~Mouse() = default;

	Vec2ui position() const { return mouseContext_.position(); }
	bool pressed(MouseButton button) const { return mouseContext_.pressed(button); }
	MouseContext& context() const { return mouseContext_; }
	Window* over() const;

	///Will be called every time a mouse button is clicked or released.
	Callback<void(MouseButton button, bool pressed)>& onButton;

	///Will be called every time the mouse moves.
	Callback<void(const Vec2ui& pos, const Vec2ui& delta)>& onMove;

	///Will be called every time the pointer focus changes.
	///Note that both parameters might be a nullptr
	///It is guaranteed that both parameters will have different values.
	Callback<void(Window* prev, Window* now)> onFocus;

protected:
	MouseContext& mouseContext_;
	App& app_;
};

}
