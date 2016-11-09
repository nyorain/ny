Developement documentation
==========================

This includes developement concepts, future ideas and interfaces or documentation for the
implementation stuff behind the scenes that is mainly interesting for ny devs.

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
///Abstract base class for handling WindowContext events.
///This is usually implemented by applications and associated with all window-relevant
///state such as drawing contexts or widget logic.
class WindowListener
{
public:
	enum class Hit
	{
		none,
		resize,
		move,
		close,
		max,
		min
	};

	struct HitResult
	{
		Hit hit;
		WindowEdges resize;
	};

	enum class OfferResult
	{
		none,
		copy,
		move,
		ask
	};

	///Returns a default WindowImpl object.
	static constexpr WindowListener& defaultInstance();

public:
	///Custom implement this function for client side decorations
	virtual HitResult click(nytl::Vec2i pos) { return Hit::none; }

	///Override this function to make the Window accept offered data objects (e.g. dragndrop).
	///Only if this function returns something other than none a DataOfferEvent can
	///be generated.
	virtual OfferResult offer(nytl::Vec2i pos, const DataTypes&) { return OfferResult::none; }

	virtual void draw() {}; ///Redraw the window
	virtual void close() {}; ///Close the window at destroy the WindowContext

	virtual void position(const PositionEvent&) {}; ///Window was repositioned
	vritual void resize(const SizeEvent&) {}; ///Window was resized
	virtual void state(const ShowEvent&) {}; ///The Windows state was changed

	virtual void key(const KeyEvent&) {}; ///Key event occurred while the window had focus
	virtual void focus(const FocusEvent&) {}; ///Window lost/gained focus

	virtual void mouseButton(const MouseButtonEvent&) {}; ///MouseButton was clicked on window
	virtual void mouseMove(const MouseMoveEvent&) {}; ///Mouse moves over window
	virtual void mouseWheel(const MouseWheelEvent&) {}; ///Mouse wheel rotated over window
	virtual void mouseCross(const MouseCrossEvent&) {}; ///Mouse entered/left window

	virtual void dataOffer(const DataOfferEvent&) {}; ///Window received a DataOffer

	///This callback is called for backend-specific events that might be interesting for
	///the applications.
	///Use this function to check for different events are only sent one backend if they
	///should be handled by the application.
	virtual void backendEvent(const Event&) {};
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
	virtual nytl::Connection fdCallback(int fd, unsigned int events, cosnt FdCallback& func) = 0;

	///Returns a number of file descriptors and their associated events that have to be monitored
	///if the applications only wants to call the event dispatch functions if it is needed.
	///So if for any of the returned file descriptors (pair.first) the associated event is met
	///(pair.second) the backend has potentially work to do.
	///Note that this call also returns all registered fds by fdCallback.
	virtual std::vector<std::pair<int, unsigned int>> fd() const = 0;
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
