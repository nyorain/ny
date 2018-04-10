// Copyright (c) 2015-2018 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/surface.hpp> // ny::Surface
#include <ny/dataExchange.hpp> // ny::DataOffer
#include <nytl/vec.hpp> // nytl::Vec
#include <nytl/flags.hpp> // ny::WindowEdges (nytl::Flags)
#include <nytl/clone.hpp> // nytl::Cloneable

#include <string> // std::string
#include <memory> // std::unique_ptr

namespace ny {

/// Classes derived from the EventData class are used by backends to put their custom
/// information (like e.g. native event objects) in Event objects, since every Event stores
/// an owned EventData pointer.
/// Has a virtual destructor which makes RTTI possible (i.e. checking for backend-specific types
/// using a dynamic_cast). EventData objects can be cloned, i.e. they can be duplicated without
/// having to know their exact (derived, backend-specific) type using nytl::clone.
/// This may be used later by the backend to retrieve which event triggered a specific call
/// the application made (event-context sensitive functions like WindowConetxt::beginMove or
/// AppContext::startDragDrop take a EventData parameter).
struct EventData : public nytl::Cloneable<EventData> {};

/// Base Event class that holds an optional EventData pointer.
/// Note that the eventData pointer is not owned and therefore must be cloned
/// if a copy if needed (copy the Event struct will simply copy the pointer).
struct Event {
	const EventData* eventData {}; /// Backend specific data associated with an event
};

/// Event that is sent when the mouse moves.
struct MouseMoveEvent : public Event {
	nytl::Vec2i position {}; /// The new mouse position
	nytl::Vec2i delta {}; /// The delta to the previous position
};

/// Event that is sent when a mouse button gets pressed or released.
struct MouseButtonEvent : public Event {
	nytl::Vec2i position {}; /// The mouse position when the event occurred
	MouseButton button {}; /// The pressed/released button
	bool pressed {}; /// True if the button was pressed, false if it was left
};

/// Event that is sent when the mouse wheel gets scrolled.
struct MouseWheelEvent : public Event {
	nytl::Vec2i position {}; /// The mouse position when the event occurred
	float value {}; /// The value of the wheel scrolling
};

/// Event that is sent when the mouse enters or leaves a window.
struct MouseCrossEvent : public Event {
	nytl::Vec2i position {}; /// Position at which the mouse entered or left
	WindowContext* other {}; /// The other (entered/left) windowContext
	bool entered {}; /// True if the WindowContext was entered, false if it was left
};

/// Event that is sent when a window receivs or loses (keyboard) focus.
struct FocusEvent : public Event {
	bool gained {}; /// True if focus was gained, false if it was lost.
};

/// Event that is sent when a key on the keyboard is pressed or released.
struct KeyEvent : public Event {
	std::string utf8 {}; /// The utf8-encoded meaning of this keypress. Empty for special keys.
	Keycode keycode {}; /// The keycode of the associated key.
	bool pressed {}; /// True if the key was pressed, false if it was released.
	bool repeat {}; /// Whether the key press represents only a repeat
	KeyboardModifiers modifiers {}; /// The active modifiers while pressing the key
};

/// Event for a window getting resized.
struct SizeEvent : public Event {
	nytl::Vec2ui size {}; /// The new size of the window
	WindowEdges edges {}; /// The edges that were resized or WindowEdge::none if unknown
};

/// Event for a window that changed its state.
struct StateEvent : public Event {
	ToplevelState state {}; /// The new ToplevelState of the window
	bool shown {}; /// Whether the window is shown
};

/// Event that is sent when a drag and drop offer enters the window
/// The associated DataOffer pointer is only valid until a DndLeaveEvent for the offer is sent.
struct DndEnterEvent : public Event {
	nytl::Vec2i position {}; /// The position at which it entered
	DataOffer* offer {}; /// The associated DataOffer
};

/// Event that is sent when a drag and drop offer moves inside the window.
/// Should be used by WindowListeners to give visual feedback if the offer can be dropped
/// at the current position, e.g. by redrawing the window.
/// The associated DataOffer pointer is only valid until a DndLeaveEvent for the offer is sent.
struct DndMoveEvent : public Event {
	nytl::Vec2i position {}; /// The new position of the dnd offer

	/// The associated DataOffer. Can be used to determine the dnd feedback
	/// (i.e. whether this DataOffer can be dropped in the window).
	DataOffer* offer {};
};

/// Event that is sent when a drag and drop offer leaves the window.
/// After processing the evnet, the DataOffer should no longer be accessed in any way.
struct DndLeaveEvent : public Event {
	DataOffer* offer {};
};

/// Event that is sent when a drag and drop offer was dropped on the window.
/// The Application should move from the DataOffer object if it wishes to take its ownership.
/// Otherwise, it will be destructed with this structure.
struct DndDropEvent : public Event {
	nytl::Vec2i position {}; /// The position at which it was dropped

	/// The associated DataOffer. Guaranteed to be the same as in the 
	/// previous dnd events. Ownership is handed over to the WindowListener.
	std::unique_ptr<DataOffer> offer {};
};

/// Event that is sent when a window should be redrawn
struct DrawEvent : public Event {};

/// Event for a window that should be closed.
struct CloseEvent : public Event {};

/// Event for a destroyed associated surface.
/// Sent if e.g. the retrieved buffer, egl or vulkan surface can no
/// longer be used.
/// It must not be further accessed and all associated resources destroyed after
/// the callback listener for this function returns.
/// The handler must also make sure that e.g. gl surfaces are made uncurrent.
/// Will not be sent on WindowContext destruction.
/// This might happen e.g. for the android backend when the native window
/// is temporarily closed.
struct SurfaceDestroyedEvent : public Event {};

/// This event is sent when a new buffer, egl or vulkan surface is created.
/// Will only be sent after a SurfaceDestroyedEvent was sent.
/// Can be used to directly retrieve the new surface which can be used
/// until the associated WindowContext is destroyed or the next SurfaceDestroyedEvent
/// is received. Will not be sent on WindowContext creation.
/// The surface is guaranteed to hold a valid implementation of the surface type
/// the WindowContext was created with.
/// This might happen e.g. for the android backend when the native window
/// is restored after beeing closed.
struct SurfaceCreatedEvent : public Event {
	Surface surface;
};

} // namespace ny
