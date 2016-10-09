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
	WindowContext* over() const override;

	//specific
	// void over(WindowContext* ctx);

protected:
	X11AppContext& appContext_;
	X11WindowContext* over_ = nullptr;
	std::bitset<8> buttonStates_;
};


///X11 KeyboardContext implementation
class X11KeyboardContext : public XkbKeyboardContext
{
public:
	X11KeyboardContext(X11AppContext& ac);
	~X11KeyboardContext() = default;

	//KeyboardContext impl
	bool pressed(Keycode key) const override;
	WindowContext* focus() const override { return focus_; }

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
	bool updateKeymap();

protected:
	X11AppContext& appContext_;
	WindowContext* focus_ = nullptr;
	std::bitset<255> keyStates_;
	std::uint8_t eventType_;
};

}

