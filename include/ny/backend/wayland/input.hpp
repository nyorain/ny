#pragma once

#include <ny/backend/wayland/include.hpp>
#include <ny/backend/common/xkb.hpp>
#include <ny/backend/mouse.hpp>
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
	Vec2ui position() const override;
	bool pressed(MouseButton button) const override;
	WindowContext* over() const override { return over_; }

	//specific
	//those functions are called by the listeners
	void handleMotion(unsigned int time, const Vec2ui& pos);
	void handleEnter(unsigned int serial, wl_surface& surface, const Vec2ui& pos);
	void handleLeave(unsigned int serial, wl_surface& surface);
	void handleButton(unsigned int serial, unsigned int time, unsigned int button, bool pressed);
	void handleAxis(unsigned int time, unsigned int axis, int value);

	wl_pointer* wlPointer() const { return wlPointer_; }

protected:
	WaylandAppContext& appContext_;
	WindowContext* over_;
	Vec2ui position_;
	wl_pointer* wlPointer_;
};


///Wayland KeyboardContext implementation
class WaylandKeyboardContext : public XkbKeyboardContext
{
public:
	WaylandKeyboardContext(WaylandAppContext& ac, wl_seat& seat);
	~WaylandKeyboardContext();

	bool pressed(Key key) const override;
	WindowContext* focus() const override { return focus_; }

	//specific
	///Returns whether the context retrieved a keymap from the compositor or not.
	///If it did not, one has to pass every key event (press/release) to it, otherwise
	///it is enough to just pass changed modifiers.
	bool keymap();

	//those functions are called by the listeners
    void handleKeymap(unsigned int format, int fd, unsigned int size);
    void handleEnter(unsigned int serial, wl_surface& surface, wl_array& keys);
    void handleLeave(unsigned int serial, wl_surface& surface);
    void handleKey(unsigned int serial, unsigned int time, unsigned int key, bool pressed);
    void handleModifiers(unsigned int serial, unsigned int mdepressed, unsigned int mlatched, 
		unsigned int mlocked, unsigned int group);

	wl_keyboard* wlKeyboard() const { return wlKeyboard_; }
	const std::bitset<255> keyStates() const { return keyStates_; }

protected:
	WaylandAppContext& appContext_;
	WindowContext* focus_;
	wl_keyboard* wlKeyboard_;
	std::bitset<255> keyStates_;
	bool keymap_;
};

}
