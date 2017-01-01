#pragma once

#include <ny/x11/include.hpp>
#include <ny/common/xkb.hpp>
#include <ny/mouseContext.hpp>

namespace ny
{

// TODO: something about over window change and relative move delta...
// also pass enter position when over changes?

/// X11 MouseContext implementation
class X11MouseContext : public MouseContext
{
public:
	X11MouseContext(X11AppContext& ac) : appContext_(ac) {}
	~X11MouseContext() = default;

	// - MouseContext -
	nytl::Vec2i position() const override;
	bool pressed(MouseButton button) const override;
	WindowContext* over() const override; // defined in src because return inheritance

	// - x11 specific -
	/// Processes the given event, i.e. checks if it is an mouse related event and if so,
	/// calls out the appropriate listeners and callbacks.
	/// Returns whether the given event was processed.
	bool processEvent(const x11::GenericEvent& ev);

	X11AppContext& appContext() const { return appContext_; }
	X11WindowContext* x11Over() const { return over_; }

protected:
	X11AppContext& appContext_;
	X11WindowContext* over_ = nullptr;
	std::bitset<8> buttonStates_;
	nytl::Vec2i lastPosition_; //synced position
};


/// X11 KeyboardContext implementation
class X11KeyboardContext : public XkbKeyboardContext
{
public:
	X11KeyboardContext(X11AppContext& ac);
	~X11KeyboardContext() = default;

	// - KeyboardContext -
	bool pressed(Keycode key) const override;
	WindowContext* focus() const override; // defined in src because return inheritance

	// - x11 specific -
	/// Returns the xkb even type id. Events with this id should be passed to
	/// processXkbEvent.
	std::uint8_t xkbEventType() const { return eventType_; }

	/// Processes the given xcb event and checks if it is keyboard related and if so,
	/// calls the appropriate listeners and callbacks.
	/// Also handles xkb specific events.
	/// Returns whether the given event was processed.
	bool processEvent(const x11::GenericEvent& ev);

	bool updateKeymap();

	X11AppContext& appContext() const { return appContext_; }

protected:
	X11AppContext& appContext_;
	X11WindowContext* focus_ {};
	uint8_t eventType_ {};
};

}
