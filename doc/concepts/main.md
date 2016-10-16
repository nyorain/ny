Editors notes:
 - This text is really unstructured and maybe only makes sense when read entirely.
 - TODO: restructure it.

The goal
========

The goal of ny is to offer a modern, independent and lightweight toolkit for all windowing uses.
It aims for gaming/cad/general-hardware-accelerated applications as well as normal ui applications.
This is achieved by offering integration with drawing capabilities while keeping the connection
as small as possible. Ny can be compiled without any drawing toolkits at all.

The goal is not:
	- fully integrated drawing library
	- complex ui/widget library

Model of operation - Threads
============================

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
threads in its interface implementation. Backends may get their events from the underlaying window
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

Hints, decoration and functionality
===================================

In ny's model of operations, the program can either request the window manager to enable/disable
certain functionality of the window. All of these requests may be ignored or simply not answered
by the backend. Nevertheless, the application can then query which functionality is enabled and
which is not. Most of those things are backend dependent.
This is needed because on some backends, there is no such concept as a floating window (i.e.
android, wayland or tiling window managers) and therefore the position of the windows cannot be
changed by the application.
The program can either request to decorate the window itself and/or specify the functionality the
window manager should allow for the window. If one wants to create a window that functions only
as temporary popup it should e.g. not have minimize or maximize functionality.
Therefore the application sets the WindowSettings::hints member when creating the window (or
using WindowContext::addHints or WindowContext::removeHints) matching its needs and intentions.
Then it can use WindowContext::capabilities to check what the backend/window manager offers in
the end.

Why Runtime polymorphism
========================

Some toolkits (e.g. sfml) use compile time switches for different backends (don't confuse this
with the runtime polymorphism e.g. by gtk which is simply hidden). The reason ny uses runtime
polymorphism (i.e. WindowContext class with a virtual table) is that some platforms have multiple
possible backends today (e.g. linux might have wayland, x11, mir, drm/kms and fbdev backends)
and therefore one could not have one executable for all backends.
The cost of using runtime polymorphism is that high that one could use this as reason for not
supporting multiple backends in one executable (at least for ny, most other toolkits that use
compile-time switches have good reason to do so).

OpenGL contexts
===============

Backend implementations should try to use as few contexts as possible.
The only reason to create a new context should be if it is needed because all existent
contexts would be incompatible with the window to create a context for (or with its settings)
or if explicitly called so by the application. Usually one opengl context per backend is enough,
even when multiple opengl windows are used.
The user could specify to use a seperate conetxt for every window e.g. if the windows are
rendered to from different threads. Note that ny with evg does not support rendering from
multiple threads (but since ny was designed to be well-usable without any evg dependency,
applications that need this error-prone feature should be able to implement it for themselves).
Since creating a context is considered pretty expensive, backends should generally only
create them when needed (for a window) and not e.g. on AppContext creation.

The usual solution to this problem (e.g. used by egl and soon reworked in wgl/glx) is to
have a raw context (maybe guarded using RAII) and implemente the actual GlContext interface
with a non-owned reference to this context and a surface/WindowContext on which the
context should be made current on a call to makeCurrentImpl.
For every window (that supports the already created raw context) there just has to be
a new wrapper GlContext implementation be created that associates the raw context with
the specific surface.

In future (on the todo list) there might be the possibility to explicitly create a 
new raw context for a window on creation which could be useful when e.g. trying to
render multiple windows in multiple threads at the same time (would not work with the
used mulitple-wrappers-around-one-context approach since a context may not be current
in multiple threads).

Vulkan
======

Applications can use ny to just create a VkSurfaceKHR for a WindowContext in a
platform-independent manner. In future further integrations for vulkan contexts are planned, like
e.g. skia or other vector graphics libraries that can use a vulkan backend.
If using ny for a larger vulkan application (e.g. game or cad) one has therefore the possiblity
to simply use ny to create a window and then a VkSurfaceKHR for it platform-independent without
any unneeded dependencies and features.

When creating the vulkan instance that ny should create the VkSurfaceKHR for, one must activate
all needed vulkan extensions by the specific backend. These can be checked with the
ny::AppContext::vulkanExtensions function of the associated AppContex implementation.

Images
======

Since ny also abstracts things like clipboards or drag-and-drop some backends need image
encode/decode functionality and therefore ny has an image class with load/save functions altough
it would usually not fit into its scope.

Keyboard input
==============

There are two entry points for keyboard input: events and the KeyboardContext interface.
Events are sent to the registered EventHandler while the KeyboardContext implementation
can be retrieved on demand from an AppContext implementation and then be used to 
check e.g. if certain keys are pressed, which WindowContext is currently focused or to get
unicode representations of keycodes.
Since a KeyboardContext does obviously represent a single keyboard and there is no possibility
to get multiple objects, there is no may to deal with multiple keyboards or differentiate 
between them at the moment. Such capability might be added later but for most backends
it would be useless since the backend apis do not support multiple keyboard differentiation
as well.

One of the main goals of ny is to abstract input across multiple platforms.
A huge part of this is to provide a modern and unicode-aware keyboard input abstraction that
can be used for all use cases.
The problem is that the keyboard can be used in many situtations, in some of them the application
cares about modifiers and the resulting unicode value and in other it does explicitly not need them
(like e.g. in games where modifiers will usually not change the key controls).
Therefore ny abstracts keyboard input on a pretty low, but cross-platform level.

Example code:
------------

``` cpp
void handlePress(const ny::KeyEvent& event, const ny::KeyboardContext& kbdctx)
{
	//We could have modifier-independet controls e.g. in a game:
	//we simply look up which action (e.g. move forward) is associated with the given keycode
	//and then execute that action.
	//All of this is totally independent from any keymap or modifiers.
	auto action = controls.actionForKeycode(event.keycode);
	if(action)
	{
		//We could also retrieve a default keysym that is associated with the given
		//keycode from the KeyboardContext. In this case we do so independent from the
		//current keyboard state (i.e. modifiers or dead pending keys).
		//If the keysym is a special key we can also retrieve a name describing it.
		auto unicode = kbdctx.utf8(event.keycode, false);
		if(unicode.empty()) ny::log("Control key ", ny::keycodeName(event.keycode));
		else ny::log("Control unicode ", unicode, " pressed");

		//perform the action
		performAction(action);
		return;
	}

	//Or just wait for unicode input e.g. for a textfield:
	//We first check whether the key does actually generate any unicode input for the current
	//keyboard state.
	//Note that for special keys (like escape) there will no unicode value be generated although
	//they have a unique unicode value since this should be only used for real char-like input.
	if(!event.unicode.empty())
	{
		//Simply further process the given unicode
		handleUnicodeInput(event.unicode);
		return;
	}
}
```

When e.g. storing keyboard controls for a game in a file one should usually store the keycode
as 32 bit integer. Note that this approach has the effect that if the used keymap is changed
in between two application startups, the unicode value of the key associated with the
control will change (i.e. from 'Y' to 'Z' when switching between german/us layout). 
The control mappings to the raw hardware keys, however, will stay the same.


Backend-specific - Wayland
==========================

Current cursor implementation (not optimal, to be changed):
- every WindowContext has its own cursor surface and wayland::ShmBuffer
- the ShmBuffer is only used if the cursor image is a custom image
- Every time the pointer enters the WindowContext, the AppContext signals the WindowContext,
	which then sets the cursor

New:
- every WindowContext has its own wayland::ShmBuffer that is used for custom image cursors
- every WindowContext has a non-owned wl_buffer* that holds the cursor contents for its surface
- WaylandMouseContext has a wl_surface* that is always the cursor surface