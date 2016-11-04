#pragma once

#include <ny/x11/include.hpp>
#include <ny/common/xkb.hpp>
#include <ny/mouseContext.hpp>

//argh...
#include <xcb/xcb.h>

namespace ny
{

///X11 MouseContext implementation
class X11MouseContext : public MouseContext
{
public:
	X11MouseContext(X11AppContext& ac) : appContext_(ac) {}
	~X11MouseContext() = default;

	//MouseContext
	Vec2ui position() const override;
	bool pressed(MouseButton button) const override;
	WindowContext* over() const override; //defined in src because X11WC inheritance

	//custom
	//TODO: something about over change and relative move delta...
	//also pass enter position when over changes?
	
	//Updates the current mouseOver WindowContext and calls the onFocus callback.
	void over(X11WindowContext* ctx);

	//Updates the given MouseButton state.
	void mouseButton(MouseButton, bool pressed);

	//Updates the synced position and calls the onMove callback.
	void move(const nytl::Vec2ui& pos);

protected:
	X11AppContext& appContext_;
	X11WindowContext* over_ = nullptr;
	std::bitset<8> buttonStates_;
	Vec2ui lastPosition_; //synced position
};


///X11 KeyboardContext implementation
class X11KeyboardContext : public XkbKeyboardContext
{
public:
	X11KeyboardContext(X11AppContext& ac);
	~X11KeyboardContext() = default;

	//KeyboardContext impl
	bool pressed(Keycode key) const override;
	WindowContext* focus() const override; //defined in src because X11WC return inheritance

	//custom
	///Returns the xkb even type id. Events with this id should be passed to
	///processXkbEvent.
	std::uint8_t xkbEventType() const { return eventType_; }	

	///Processed xkb server events to e.g. update the keymap
	void processXkbEvent(xcb_generic_event_t& ev);

	///Updates the current focused WindowContext and calls the onFocus callback.
	void focus(X11WindowContext* now);
	bool updateKeymap();

protected:
	X11AppContext& appContext_;
	X11WindowContext* focus_ {};
	std::uint8_t eventType_ {};
};

}

