// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/event.hpp> // ny::Event
#include <ny/dataExchange.hpp> // ny::DataFormat

namespace ny {

/// Abstract base class for handling WindowContext events.
/// This is usually implemented by applications and associated with all window-relevant
/// state such as drawing contexts or widget logic.
class WindowListener {
public:
	/// Returns a default WindowImpl object.
	/// WindowContexts without explicitly set WindowListener have this object set.
	/// This is done so it hasn't to be checked everytime whether a WindowContext has a valid
	/// WindowListener.
	static WindowListener& defaultInstance() { static WindowListener ret; return ret; }

public:
	/// This function is called when a dnd action enters the window.
	/// The window could use this to e.g. redraw itself in a special way.
	/// Remember that after calling a dispatch function in this function, the given
	/// DataOffer might not be valid anymore (then a dndLeave or dndDrop event occurred).
	virtual void dndEnter(const DndEnterEvent&) {}

	/// Called when a previously entered dnd offer is moved around in the window.
	/// Many applications use this to enable e.g. scrolling while a dnd session is active.
	/// Should return whether the given DataOffer could be accepted at the given position.
	/// Remember that after calling a dispatch function in this function, the given
	/// DataOffer might not be valid anymore (then a dndLeave or dndDrop event occurred).
	virtual DataFormat dndMove(const DndMoveEvent&) { return {}; };

	/// This function is called when a DataOffer that entered the window leaves it.
	/// The DataOffer object should then actually not be used anymore and is just passed here
	/// for comparison. This function is only called if no drop occurs.
	virtual void dndLeave(const DndLeaveEvent&) {}

	/// Called when a dnd DataOffer is dropped over the window.
	/// The application gains ownership about the DataOffer object.
	/// This event is only received when the previous dndMove handler returned true.
	virtual void dndDrop(const DndDropEvent&) {}

	virtual void draw(const DrawEvent&) {} /// The window should be redrawn
	virtual void close(const CloseEvent&) {} /// Close the window at destroy the WindowContext
	virtual void destroyed() {} /// Informs the listener that the WindowContext was destroyed

	virtual void resize(const SizeEvent&) {} /// Window was resized
	virtual void state(const StateEvent&) {} /// Window state changed

	virtual void key(const KeyEvent&) {} /// A key was pressed or released
	virtual void focus(const FocusEvent&) {} /// Window gained or lost focus

	virtual void mouseButton(const MouseButtonEvent&) {} /// A mouse button was pressed or released
	virtual void mouseMove(const MouseMoveEvent&) {} /// Mouse moved over window
	virtual void mouseWheel(const MouseWheelEvent&) {} /// Mouse wheel rotated over window
	virtual void mouseCross(const MouseCrossEvent&) {} /// Mouse entered/left window

protected:
	WindowListener() = default;
	virtual ~WindowListener() = default;
};

} // namespace ny
