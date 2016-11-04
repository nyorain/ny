#pragma once

#include <ny/wayland/include.hpp>
#include <ny/common/xkb.hpp>
#include <ny/mouseContext.hpp>
#include <nytl/vec.hpp>

namespace ny
{

///Wayland MouseContext implementation
class WaylandMouseContext : public MouseContext
{
public:
	WaylandMouseContext(WaylandAppContext& ac, wl_seat& seat);
	~WaylandMouseContext();

	//MouseContext
	Vec2ui position() const override { return position_; }
	bool pressed(MouseButton button) const override;
	WindowContext* over() const override;

	//specific
	wl_pointer* wlPointer() const { return wlPointer_; }
	unsigned int lastSerial() const { return lastSerial_; }

	///This function does only actively change the cursor if it is currently over
	///an owned surface or grabbed by this application.
	///\param buf Defined the cursor content. If nullptr, the cursor is hidden
	///\param hs The cursor hotspot. {0, 0} would be the top-left corner.
	///\param size The cursor size. Only relevant for damaging the surface so should be
	///any value equal to or greater than the actual buffer size.
	void cursorBuffer(wl_buffer* buf, nytl::Vec2i hs = {}, nytl::Vec2ui size = {2048, 2048}) const;

protected:
	WaylandAppContext& appContext_;
	WaylandWindowContext* over_ {};
	Vec2ui position_;
	wl_pointer* wlPointer_ {};
	std::bitset<8> buttonStates_;

	wl_surface* wlCursorSurface_ {};

	unsigned int lastSerial_ {};
	unsigned int cursorSerial_ {};

protected:
	void handleEnter(unsigned int serial, wl_surface*, wl_fixed_t x, wl_fixed_t y);
	void handleLeave(unsigned int serial, wl_surface*);
	void handleMotion(unsigned int time, wl_fixed_t x, wl_fixed_t y);
	void handleButton(unsigned int serial, unsigned int time, unsigned int button, bool pressed);
	void handleAxis(unsigned int time, unsigned int axis, wl_fixed_t value);
	void handleFrame();
	void handleAxisSource(unsigned int source);
	void handleAxisStop(unsigned int time, unsigned int axis);
	void handleAxisDiscrete(unsigned int axis, int discrete);
};


///Wayland KeyboardContext implementation
class WaylandKeyboardContext : public XkbKeyboardContext
{
public:
	WaylandKeyboardContext(WaylandAppContext& ac, wl_seat& seat);
	~WaylandKeyboardContext();

	bool pressed(Keycode key) const override;
	WindowContext* focus() const override { return focus_; }

	//specific
	bool withKeymap(); //whether the compositor sent a keymap

	wl_keyboard* wlKeyboard() const { return wlKeyboard_; }
	unsigned int lastSerial() const { return lastSerial_; }

protected:
	WaylandAppContext& appContext_;
	WindowContext* focus_ {};
	wl_keyboard* wlKeyboard_ {};
	bool keymap_ {};
	unsigned int lastSerial_ {};

protected:
    void handleKeymap(unsigned int format, int fd, unsigned int size);
    void handleEnter(unsigned int serial, wl_surface* surface, wl_array* keys);
    void handleLeave(unsigned int serial, wl_surface* surface);
    void handleKey(unsigned int serial, unsigned int time, unsigned int key, bool pressed);
    void handleModifiers(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
	void handleRepeatInfo(int rate, int delay);
};

}
