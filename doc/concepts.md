Developement concepts
==========================

*__This includes developement concepts, or future ideas and interfaces.__*

EventTypeRegister
=================

The idea of a static event type register that totally prevents multiple events having the same
number. The problem would be that multiple processes could have different numbers for
different event types. Solution: make them serializable using names.
The numbers/names could be generated/claimed statically (since EventTypeRegister would be
a singleton) as well as at runtime dynamically.

```cpp

class EventTypeRegister
{
public:
	EventTypeRegister& instance();

public:
	std::string name(unsigned int type);
	unsigned int id(const std::string& name);

	bool registered(unsigned int type);
	unsigned int register(const std::string& name, unsigned int pref = 0);
	unsigned int registerPref(const std::string& name, unsigned int pref);
};

static auto eventTypeClose = EventTypeRegister::instance().register("ny::eventType::close");

```

WindowListener
==============

Concept that aims to replace the whole need for EventHandlers and the rather complex
event dispatching mechanisms. The backend implementations just call directly those
functions if a WindowContext has a registered WindowListener.

```cpp
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
	static constexpr WindowListener& defaultInstance();

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

	virtual void key(bool pressed, Key key, std::string utf8, const EventData*);
	virtual void focus(bool shown, ToplevelState state, const EventData*);

	virtual void mouseButton(bool pressed, MouseButton button, EventData*);
	virtual void mouseMove(nytl::Vec2i position, const EventData*) {}; ///Mouse moves over window
	virtual void mouseWheel(int value, const EventData*) {}; ///Mouse wheel rotated over window
	virtual void mouseCross(bool focused, const EventData*) {}; ///Mouse entered/left window
};

```

### BackendEvent

```cpp
///Part of WindowListener
class WindowListener
{
    ...

	///This callback is called for backend-specific events that might be interesting for
	///the applications. This function should only be used by applications that want to offer
    ///platform-specific features.
    ///It can then check whether a certain backend is used and cast the received BackendEvent ///to the derived BackendEvent types of the specific backend.
	virtual void backendEvent(const BackendEvent&) {};
};

///Backend-specific event that may be handled by the application.
///Contains usually events that may have an effect on the application but are only
///emitted by the one backend like e.g. x11 reparent events, wayland registry events.
class BackendEvent
{
public:
    virtual ~BackendEvent() = default;
};
```

Stub functions
==============

#### The current situation:
If an application is developed on a system where e.g. ny is installed without wayland support,
the application will not be able to call any wayland-specific functionality (or Otherwise
compiling/linking the application will fail).
Even if it eventually executed on a platform that has ny with gl support (and the library
with the defined ny::wayland symbols is loaded), it will not be able to use it
since it was compiled without any wayland functionality.

#### The idea:
To make applications compile with ny functionality that is supported on the compiling platform
but might be present on the executing platform, ny could provide stub functions for
functionality it is built without.
This would mean if ny is built on windows (e.g. without wayland), it would nontheless
include stub function for all wayland functions defined in the ny wayland headers.
Those stub functions could e.g. throw an exception that the backend is not supported.
If the application is then executed on another platform where the real implemented
functionality is present, it would call these instead of the stub functions (since the stub
functions would obviously only be built-in when the real functions are not).
This would completely eliminate the need for compile time configuration checking and applications
would only have to check the build configuration of ny on the executing platform.

#### Possible downsides:
- Symbol bloat of the ny library. Even ny compiled on windows would e.g. have the
WaylandWindowContext symbols defined which could lead to some problems/downsides.
- Extra maintain work since every time a new function is added a new stub has to be added
- Different from how libraries are doing it usually so it might be unintuitive
- Is it really needed? How important is this? distributed apps will simply have to be built
on platform where are features are present, this is how it is already usually done


Common Unix abstraction
=======================

The idea is to have a common abstraction above all unix backends to make integrating it
with already existing applications/libaries easier.
All unix backends have in common that they somehow operator on file descriptors (since
this is the usual way for communicating with other application like e.g. a window manager)
and therefore the main aspect of the unix abstraction deals with file descriptors which
can be really powerful for applications.

```cpp
///Gives unix applications the possibility to integrate ny with existing event loops or
///other file descriptors that have to be monitored.
///Note that this abstraction does only apply to unix since there is no equivalent of the same
///value and functionality as file descriptors for non-unix (i.e. windows) backends.
///The AppContext implementations derived by this abstract base class do usually already
///make good use of the fdCallback functionality e.g. for timers, filesystem monitoring or
///simply synchronization.
class UnixAppContext : public AppContext
{
public:
	using FdCallback = nytl::CompFunc<void(nytl::ConnectionRef, int fd, unsigned int events)>;

public:
	///The event dispatching functions will now monitor the given file descriptor for the given
	///events. If one of the events is met, calls the given callback function.
	///If monitoring the fd is no longer needed or the fd is about to get closed, unregister
	///it by disconnecting the return Connection.
	///Should only be called from the gui thread.
	virtual nytl::Connection fdCallback(int fd, unsigned int events, cosnt FdCallback& func) = 0;

	///Returns a number of file descriptors and their associated events that have to be monitored
	///if the applications only wants to call the event dispatch functions if it is needed.
	///So if for any of the returned file descriptors (pair.first) the associated event is met
	///(pair.second) the backend has potentially work to do.
	///Note that this call also returns all registered fds by fdCallback.
	///Should only be called from the gui thread.
	virtual std::vector<std::pair<int, unsigned int>> fds() const = 0;
};
```

Window Additions
========================

Ideas for additional WindowContext/WindowSettings features.

Additional capabitlity suggestions:
-----------------------------------

 - change cursor
 - change icon
 - make droppable
 - beginMove
 - beginResize
 - title

WindowType
----------

Specifies the type of the window, i.e. the intention of its display.
This might change how the window is presented to the user by the backend.

```cpp
enum class WindowType : unsigned int
{
	none =  0,
	toplevel,
	dialog,
	//...
};
```

### DialogSettings

DialogSettings, to enable some kind of native dialogs.
E.g. on windows native file/color/font picker dialogs. But since they arent even used
on windows anymore tbh, would this really make sense?

AppContextSettings
==================

Could be useful for displaying the applications name correctly and automatically parse
arguments (introduce ny-specific arguments).
Is this useful, inside of the scope of ny?

```cpp
struct AppContextSettings
{
	std::string name;
	bool multithreaded;
	nytl::Range<const char*> args;
	std::vector<std::pair<const char*, const char*>> licenses;
	const char* author;
};
```

DataActions
-----------

Give DataOffer and DataSource the possiblity for providing/chosing DataActions.
Also give the possibilty for accepting a data format, inspecting it and give a feedback
before the data is dropped (connect with WindowImpl).

```cpp
enum class DataAction
{
	none,
	move,
	copy,
	ask
};
```

Special position/size values
============================

```cpp
///Used as magical signal value for no postion.
constexpr nytl::Vec2i noPosition = {INT_MAX, INT_MAX};

///Used as magical signal value for no size.
constexpr nytl::Vec2ui noSize = {UINT_MAX, UINT_MAX};
```
