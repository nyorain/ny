// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <nytl/clone.hpp>
#include <nytl/vec.hpp>

//This header and its functionality can be used without linking to ny.

namespace ny
{

///Classes derived from the EventData class are used by backends to put their custom
///information (like e.g. native event objects) in Event objects, since every Event stores
///an owned EventData pointer.
///Has a virtual destructor which makes RTTI possible (i.e. checking for backend-specific types
///using a dynamic_cast). EventData objects can be cloned, i.e. they can be duplicated without
///having to know their exact (derived, backend-specific) type.
///This may be used later by the backend to retrieve which event triggered a specific call
///the application made (event-context sensitive functions like WindowConetxt::beginMove or
///AppContext::startDragDrop take a EventData parameter).
class EventData : public nytl::Cloneable<EventData>
{
public:
	virtual ~EventData() = default;
};

///Abstract base class for handling WindowContext events.
///This is usually implemented by applications and associated with all window-relevant
///state such as drawing contexts or widget logic.
class WindowListener
{
public:
	///Returns a default WindowImpl object.
    ///WindowContexts without explicitly set WindowListener have this object set.
    ///This is done so it hasn't to be checked everytime whether a WindowContext has a valid
    ///WindowListener.
	static WindowListener& defaultInstance() { static WindowListener instance; return instance; }

public:
    ///This function is called when a dnd action enters the window.
    ///The window could use this to e.g. redraw itself in a special way.
    ///Remember that after calling a dispatch function in this function, the given
    ///DataOffer might not be valid anymore (then a dndLeave or dndDrop event occurred).
    virtual void dndEnter(const DataOffer&, const EventData*) {};

    ///Called when a previously entered dnd offer is moved around in the window.
    ///Many applications use this to enable e.g. scrolling while a dnd session is active.
    ///Should return whether the given DataOffer could be accepted at the given position.
    ///Remember that after calling a dispatch function in this function, the given
    ///DataOffer might not be valid anymore (then a dndLeave or dndDrop event occurred).
    virtual bool dndMove(nytl::Vec2i pos, const DataOffer&, const EventData*) { return false; }

    ///This function is called when a DataOffer that entered the window leaves it.
    ///The DataOffer object should then actually not be used anymore and is just passed here
    ///for comparison. This function is only called if no drop occurs.
    virtual void dndLeave(const DataOffer&, const EventData*) {};

    ///Called when a dnd DataOffer is dropped over the window.
    ///The application gains ownership about the DataOffer object.
    ///This event is only received when the previous dndMove handler returned true.
	virtual void dndDrop(nytl::Vec2i pos, std::unique_ptr<DataOffer>, const EventData*) {}

	virtual void draw(const EventData*) {}; ///Redraw the window
	virtual void close(const EventData*) {}; ///Close the window at destroy the WindowContext

	virtual void position(nytl::Vec2i position, const EventData*) {}; ///Window was repositioned
	virtual void resize(nytl::Vec2ui size, const EventData*) {}; ///Window was resized
	virtual void state(bool shown, ToplevelState, const EventData*) {}; ///Window state changed

	virtual void key(bool pressed, Keycode, std::string utf8, const EventData*);
	virtual void focus(bool shown, ToplevelState state, const EventData*);

	virtual void mouseButton(bool pressed, MouseButton button, EventData*);
	virtual void mouseMove(nytl::Vec2i position, const EventData*) {}; ///Mouse moves over window
	virtual void mouseWheel(float value, const EventData*) {}; ///Mouse wheel rotated over window
	virtual void mouseCross(bool focused, const EventData*) {}; ///Mouse entered/left window
};

}
