#include <ny/backend/wayland/input.hpp>
#include <ny/backend/wayland/interfaces.hpp>
#include <ny/backend/wayland/appContext.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/util.hpp>
#include <ny/backend/windowContext.hpp>
#include <ny/base/log.hpp>

#include <nytl/range.hpp>

#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>
#include <unistd.h>

namespace ny
{

using namespace wayland;

//mouse
WaylandMouseContext::WaylandMouseContext(WaylandAppContext& ac, wl_seat& seat)
	: appContext_(ac)
{
	wlPointer_ = wl_seat_get_pointer(&seat);
    wl_pointer_add_listener(wlPointer_, &pointerListener, this);
}

WaylandMouseContext::~WaylandMouseContext()
{
	wl_pointer_release(wlPointer_);
}

bool WaylandMouseContext::pressed(MouseButton button) const
{
	return buttonStates_[static_cast<unsigned int>(button)];
}

WindowContext* WaylandMouseContext::over() const
{
	return over_;
}

void WaylandMouseContext::handleMotion(unsigned int time, const Vec2ui& pos)
{
	unused(time);

	onMove(position_, pos);
	if(over_ && over_->eventHandler())
	{
		MouseMoveEvent event(over_->eventHandler());
		event.position = pos;
		event.delta = position_ - pos;
		appContext_.dispatch(std::move(event));
	}

	position_ = pos;
}
void WaylandMouseContext::handleEnter(unsigned int serial, wl_surface& surface, const Vec2ui& pos)
{
	position_ = pos;
	auto wc = appContext_.windowContext(surface);

	if(wc != over_)
	{
		onFocus(over_, wc);

		if(over_ && over_->eventHandler())
		{
			MouseCrossEvent event(over_->eventHandler());
			event.data = std::make_unique<WaylandEventData>(serial);
			event.entered = false;
			appContext_.dispatch(std::move(event));
		}
		if(wc && wc->eventHandler())
		{
			MouseCrossEvent event(wc->eventHandler());
			event.data = std::make_unique<WaylandEventData>(serial);
			event.entered = true;
			appContext_.dispatch(std::move(event));
		}

		over_ = wc;
	}
}
void WaylandMouseContext::handleLeave(unsigned int serial, wl_surface& surface)
{
	auto wc = appContext_.windowContext(surface);
	if(wc != over_)
		warning("WlMouseContext::handleLeave: over_ and wc not matching");

	if(over_ != nullptr) onFocus(over_, nullptr);
	if(wc && wc->eventHandler())
	{
		MouseCrossEvent event(wc->eventHandler());
		event.data = std::make_unique<WaylandEventData>(serial);
		event.entered = false;
		appContext_.dispatch(std::move(event));
	}

	over_ = nullptr;
}
void WaylandMouseContext::handleButton(unsigned int serial, unsigned int time, 
	unsigned int button, bool pressed)
{
	unused(time);

	auto nybutton = linuxToButton(button);
	onButton(nybutton, pressed);

	if(over_)
	{
		MouseButtonEvent event(over_->eventHandler());
		event.button = nybutton;
		event.data = std::make_unique<WaylandEventData>(serial);
		appContext_.dispatch(std::move(event));
	}
}
void WaylandMouseContext::handleAxis(unsigned int time, unsigned int axis, int value)
{
	//TODO
	unused(time, axis, value);

	if(over_)
	{
	}
}

//keyboard
WaylandKeyboardContext::WaylandKeyboardContext(WaylandAppContext& ac, wl_seat& seat)
	: appContext_(ac)
{
	wlKeyboard_ = wl_seat_get_keyboard(&seat);
    wl_keyboard_add_listener(wlKeyboard_, &keyboardListener, this);
	XkbKeyboardContext::createDefault();
}

WaylandKeyboardContext::~WaylandKeyboardContext()
{
	wl_keyboard_release(wlKeyboard_);
}

bool WaylandKeyboardContext::pressed(Key key) const
{
	return keyStates_[static_cast<unsigned int>(key)];
}

bool WaylandKeyboardContext::keymap()
{
	return keymap_;
}
void WaylandKeyboardContext::handleKeymap(unsigned int format, int fd, unsigned int size)
{
	if(format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) return;

	auto buffer = std::make_unique<char[]>(size);
	read(fd, buffer.get(), size); //TODO: check for error
	keymap_ = true;
	
	if(xkbState_) xkb_state_unref(xkbState_);
	if(xkbKeymap_) xkb_keymap_unref(xkbKeymap_);

	xkbKeymap_ = xkb_keymap_new_from_string(xkbContext_, buffer.get(),
		XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
}
void WaylandKeyboardContext::handleEnter(unsigned int serial, wl_surface& surface, wl_array& keys)
{
	keyStates_.reset();
	for(auto i = 0u; i < keys.size; ++i)
	{
		auto keyid = static_cast<std::uint32_t*>(keys.data)[i];
		auto key = linuxToKey(keyid);
		keyStates_[static_cast<unsigned int>(key)] = true;
	}
	
	auto* wc = appContext_.windowContext(surface);
	if(wc != focus_)
	{
		onFocus(focus_, wc);

		if(focus_ && focus_->eventHandler())
		{
			FocusEvent event(focus_->eventHandler());
			event.data = std::make_unique<WaylandEventData>(serial);
			event.focus = false;
			appContext_.dispatch(std::move(event));
		}
		if(wc && wc->eventHandler())
		{
			FocusEvent event(wc->eventHandler());
			event.data = std::make_unique<WaylandEventData>(serial);
			event.focus = true;
			appContext_.dispatch(std::move(event));
		}

		focus_ = wc;
	}
}
void WaylandKeyboardContext::handleLeave(unsigned int serial, wl_surface& surface)
{
	auto* wc = appContext_.windowContext(surface);
	if(wc != focus_) 
		warning("WlKeyboardContext::handleLeave: focus_ and wc not matching");

	if(wc)
	{
		onFocus(wc, nullptr);

		if(wc->eventHandler())
		{
			FocusEvent event(wc->eventHandler());
			event.data = std::make_unique<WaylandEventData>(serial);
			event.focus = false;
			appContext_.dispatch(std::move(event));
		}
	}

	keyStates_.reset();
	focus_ = nullptr;
}
void WaylandKeyboardContext::handleKey(unsigned int serial, unsigned int time, 
	unsigned int key, bool pressed)
{
	unused(time);

	auto nykey = linuxToKey(key);
	XkbKeyboardContext::updateKey(key, pressed);
	onKey(nykey, pressed);
	
	if(focus_ && focus_->eventHandler())
	{
		KeyEvent event(focus_->eventHandler());
		event.data = std::make_unique<WaylandEventData>(serial);
		event.key = nykey;
		event.unicode = unicode(nykey);
		event.pressed = pressed;
		appContext_.dispatch(std::move(event));
	}
}
void WaylandKeyboardContext::handleModifiers(unsigned int serial, unsigned int mdepressed, 
	unsigned int mlatched, unsigned int mlocked, unsigned int group)
{
	unused(serial);
	XkbKeyboardContext::updateState({mdepressed, mlatched, mlocked}, {group, group, group});
}

}
