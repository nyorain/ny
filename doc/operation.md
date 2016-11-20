Model of operation
==================

There are some special core concepts about threading, events and asynchronous operations that
every application developer using ny should understand.
The goal of ny was always to design an abstraction as flexible as possible regarding application
design.

Threads
=======

In general, ny is not designed threadsafe but thread-aware. At some points it provides
some synchronization mechanism and guarantees but usually it is focues on one single thread.

#### The Event Thread

The thread in which AppContext::dispatchEvents or AppContext::dispatchLoop or functions
calling those functions (dispatch functions) is called __Event Thread__.
There are several requirements and guarantees regarding the event thread:

	- Those functions must only ever be called from one thread (for one AppContext object).
	- All WindowContext object from the AppContext must be created in this thread
	- The AppContext will never call any listener or event handler from another thread

While the event thread is active and might be calling some functions associated with
an AppContext (i.e. some WindowContext or DataOffer function associated with the AppContext),
no other thread is allowed to call any function associated with the AppContext.

All functionality associated with Backends may not work if no disptach function is called.
So if e.g. a listener in the main thread blocks, the windows will most likely become
unreponsive and the application might be killed by the user.
Therefore no blocking functions should be called in the event thread and it must be assured
that dispatch functions are called often enough.

There are two main idioms used for event dispatching. The first one, which is mostly used
by usual gui applications is to just use the dispatchLoop. This way, it is assured that
the AppContext can always instantly handle input coming from the backend.

```cpp
///Just call the dispatch loop and exit the application if it finishes
///This function will run/wait for events until an error occurred or we exit it with
///the loopControl.
///We will probably use the loopControl to exit the loop when e.g. a window is closed.
appContext.dispatchLoop(loopControl);
return EXIT_SUCCESS;
```

The second way, usually used by games or applications that want to use the idle time for
rendering, just dispatches all events once per frame.
If it can be assured that the render function will not block in any way and really only
render one frame (which should not take that long) this will also keep the AppContext
and all associated functionality alive.

```cpp
///We will probably set run to false from some event listener, e.g. when a window is closed.
///Until then we will dispatch all events and render as often as possible.
while(run)
{
	///This function is guaranteed to not block (if no event listeners block).
	appContext.dispatchEvents();
	render();
}
```

Dispatch nesting
================

Sometimes application might want to wait for some request completion before they
return from some event listener. Since the window would become unresponsive if they would
just block, they can simply run a nested dispatch function while waiting which will keep
the AppContext functional but allow them to wait before returning.

```cpp
///This is a function called by the AppContext from within a disaptch function e.g.
///when some event is received.
void someEventListener()
{
	///For some strange reason, we want to wait until the next key
	///is pressed before we exit this function.
	ny::LoopControl lc;
	nytl::ConnectionGuard conn = appContext.keyboardContext()->onKey.add([&]{ lc.stop(); });
	appContext.dispatchLoop(lc);

	///Here we would then probably do something with the keypress...
}
```

The code above is perfectly valid, AppContext implementations have to assure that their dispatch
function can be called from within each other. Nontheless application have to assure that
they don't nest them infinitely. If in the example above the listener function can be called
from the dispatchLoop itself started, the nesting could just never end and cause something
like a stack overflow.

This pattern is especially useful for the data exchange functionality, e.g. for DataOffers.
See the [dataExchange](dataExchange.md) documentation for more information.

Behind the Scenes
-----------------

-- under construction --

Old documentation
-----------------

Events are only allowed to be sent and processed at one thread at a time.
This thread will be called the ui thread in the following. This is the thread in which the
application calls App::run, AppContext::dispatchEvents or similiar functions.
Note that those functions shall always be called from the thread in which the AppContext and
WindowContexts (or App and Windows) were created. WindowContexts should never be created from
another thread. On most backends (linux) this would be no problem, but it would need some
non-trivial effort to make it possible on windows. It is simpler to just use
event-based-communication when using multiple threads and then just create/use all windows
from the ui thread.

tl;dr:
	- All windows must be created in the same thread as their appContext (the ui thread)
	- All AppContext dispatch functions should only be called from within this thread

No backend implementation should ever call EventHandler (not even multiple ones) from multiple
threads in its implementation. Backends may get their events from the underlaying window
system or devices in this event thread or in a seperate thread.
Backend implementations also guarantee that EventHandler will only be called from inside one
of the three AppContext event dispatch functions.
If events are generated while none of these functions is called (e.g. on WindowContext
construction) the events will be queued and only sent once the dispatch functions are
called.

Note that implementing event systems similiar to sfml or different backends (where you can all a
functions to just retrieve the next queued event and then handle in manually) does not make much
sense in ny because backends usually use internal events to implement and comminucate between all
interfaces.

The program side (i.e. the programmer that uses ny) does promise that the EventHandler wont call
blocking functions that do not run its own event loop. Ny itself contains functions that may block,
but then run its own event loop while waiting for a certain event to occur. Those functions are
only allowed to be called from the ui thread.
Examples for such functions are AppContext::startDragDrop or Dialog::runModal.

Ny considers itself a thread-aware library that may be used for efficient single-threaded as well as
multi-threaded applications.
You can make your application multi-threaded by running the AppContext::dispatchLoop overload
that takes an EventDispatcher parameter and then sending events from multiple thread
to this EventDispatcher.
The way to communication method between the ui thread running the event dispatch loop and other
threads are therefore usually events. This way you are able to e.g. resize, refresh or
hide windows from other threads. Doing this directly (by calls to the functions instead of events)
may result in undefined behaviour such as a data race.
When using ny-app, you simply have to set the multithreaded bool in the AppSettings objects that you
pass on App constrcution to true.

If you want to understand all aspects of multithreading in ny on a lower level, you should really
read the documentation for EventDispatcher, and the 3 event dispatching functions in AppContext.
