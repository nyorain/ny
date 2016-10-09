#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/common/xkb.hpp>
#include <ny/backend/mouseContext.hpp>

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
	X11WindowContext* over() const override { return over_; }

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
	X11WindowContext* focus() const override { return focus_; }

	//custom
	///Returns the xkb even type id. Events with this id should be passed to
	///processXkbEvent.
	std::uint8_t xkbEventType() const { return eventType_; }	

	///Processed xkb server events to e.g. update the keymap
	void processXkbEvent(xcb_generic_event_t& ev);

	///Fills the given KeyEvent depending on the KeyEvent::pressed member and the given
	///x keycode.
	///Returns false when the given keycode cancelled the current compose state.
	bool keyEvent(std::uint8_t keycode, KeyEvent& ev);

	///Updates the current focused WindowContext and calls the onFocus callback.
	bool focus(X11WindowContext* now);
	bool updateKeymap();

protected:
	X11AppContext& appContext_;
	X11WindowContext* focus_ {};
	std::bitset<255> keyStates_;
	std::uint8_t eventType_ {};
};

}

