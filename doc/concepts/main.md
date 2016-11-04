Editor notes:
 - Documentation for devs
 - This text is really unstructured and maybe only makes sense when read entirely.
 - TODO: restructure it.

The goal
========

The goal of ny is to offer a modern, independent and lightweight toolkit for all windowing uses.
It aims for gaming/cad/general-hardware-accelerated applications as well as normal ui applications.
This is achieved by offering integration with drawing capabilities while keeping the connection
as small as possible. It can be compiled without any drawing toolkits (or other bloated
dependencies) at all.
The only things it needs are basically the needed underlaying backend libraries, a bunch
modern C++ template headers that are downloaded automatically during building (cmake + git)
and xkbcommon on linux backends (could theoretically be replaced, but using it is really sane
since it is a good, efficient, lightweight and independent library).

The goal is not:
	- fully integrated drawing library
	- complex ui/widget library
	- gaming-only windowing system

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

Custom Decoration
=================

Custom deocration refers to things like titlebar with close/maximize/minimize buttons, title
and additional button, borders or shadows drawn as part of the window.
Backends have some default setting for custom decoration (on/off), on common backends like
x11 or win32 they will be set to off since usually the window manager takes care of
decoration. On wayland (where client side decorations are common) they will be enabled
by default. The application can query whether it is expected to draw custom decoration for
the window using WindowContext::customDecorated.
Backends that have no server-side decoration, but where the user does usually not expect
decorations at all (like android or drm display) should return false.

If the application does explicitly wish for custom decorations and WindowContext::customDecorated
returns false, it can try to enable it (which should work on most backends), i.e. ask the
backend to not draw its own decorations around the window.
Notice that doing so does only make sense wihtin reason, since some users would like to
have an uniform look across all applications on their desktop. The best way to go
is probably making it possible for the user to toggle custom decorations or to just
go with default decorations.

Asking the backend for client side decorations (or for server side decorations if they are
off by default) can be done using the WindowContext::{add/remove}WindowHints functions.
Note that the result queried by ny::WindowContext::customDecorated might be wrong in
some cases (e.g. it cannot be correctly detected on an x11 backend - one only asks the server
to not decorate it, if they do so is not known) so this result should be used as sane
default while giving the user the possiblity to explicitly enable/disable client decorations
no matter what ny reports.
Example code:

```
bool customDecorated; //will hold if we should draw decorations in the end
if(windowContext.customDecorated())
{
	//the application should decorate the window by default
	customDecorated = true;

	//check if the user wishes for server side decorations
	if(!settings.customDecorations)
	{
		//ask the WindowContext/backend/server for server side decorations
		//Note that this case is usually rare and will most likely fail in practice
		//since if WindowContext::customDecorated() returns true, there
		//are most likely simply no server side decorations like e.g. for
		//most wayland compositors
		windowContext.removeWindowHints(ny::WindowHint::customDecorated);
		if(settings.customDecorated())
		{
			customDecorated = true;
			ny::log("Requested server side decorations are not supported by the backend");
		}
		else
		{
			//The application does not have to draw any decorations.
			customDecorated = false;
		}
	}
}
else
{
	//the application should not decorate the window by default
	customDecorated = false;

	//check if the user wishes for client side decorations
	if(settings.customDecorations)
	{
		//ask the WindowContext/backend/server to not draw any decorations
		//This may be done by many usual applications if they have some smart
		//custom decoration design ideas to improve the ui.
		//Note that forcing your own decorations onto the user - no matter how
		//nice they actually look - is never a good choice.
		//Like we do above, applications should always leave the choice to the user.
		windowContext.addWindowHints(ny::WindowHint::customDecorated);
		if(settings.customDecorated())
		{
			//The window is now customDecorated - decorate it!
			customDecorated = true;
		}
		else
		{
			customDecorated = false;
			ny::log("Requested client side decorations are not supported by the backend");
		}
	}
}

//If the user did explicitly set settings.customDecorations, the application should only
//repect this value and not the final value reported by ny (which is now stored in
//the variable <customDecorated>). So it might make sense to simply override
//<customDecorated> with the user preferece, as explained above.
```

Good applications should check WindowContext::customDecorated as default since on backends like
Wayland, the application might really look poor to begin with without custom decorations.
Note that in future there might be additional ways for wayland to query whether the user/server
wants client side decorations (since e.g. KDE users might not expect them) so on should
always only use the value returned from WindowContext::customDecorated and only do so
for the WindowContext it was called for.

About graphics, visuals and pixel formats
=========================================

Most window systems are really old and therefore still deal with things like colormaps
or visuals. But since today nearly all hardware has support for 24 or 32 bit images/window
contents ny does not implement full support for backends that don't support 24 or 32 bit
drawing in any way (e.g. some ancient x server/driver/hardware).

OpenGL contexts
===============

Contexts are explicitly created and managed by the application if needed.
Configs can be explicitly selected in a cross-platform way, or one can simply use the
default config which should have pretty good attributes.

If requested, a GlSurface implementation will be created for a WindowContext that can then
be used to render onto the Window. There are some platform-dependent ways to render into
a pixel buffer.

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

Backends
========

The entry point to most of nys functionality is a Backend object.
This can either be manually implemented, manually chosen or automatically be chosen by ny.
The sane default that should be used if there are no reasons to not do so it letting ny
chose the backend by using the static ny::Backend::choose function.
It will test which of the registered backends (which are the default ny backends and any number
of custom implemented or loaded backends) are available and then just chose one.
First of all the algorithm checks if the "NY_BACKEND" environment variable is set and if
there is a registered backend with the value of the variable and this backend is available it will
be chosen.

Otherwise it will chose one of the available backends, where it will prefer the built-in backends
but if none of them are available it will just chose the first unknown one.
If multiple built-in backends are availble, the function choses winapi > wayland > x11 since
x11 (and theoretically wayland) might be emulated on windows as well as x11 is usually
emulated on wayland and ny wants to chose the native backend.

If the function finds no available backend, it will throw a std::runtime_error which could
be caught by the application, but since it cannot use ny in any way in this case, the application
usually wants to exit this way.

Custom Backend implementations just have to create an instance of themselfs (which is usually
done with a static variable) to getting considered by the Backend::choose algorithm.
Notice that ny is always open for new custom backends so please consider to let ny pull
your backend implementations into its own codebase.


Backend-specific - Wayland
--------------------------

Current cursor implementation (not optimal, to be changed):
- every WindowContext has its own wayland::ShmBuffer that is used for custom image cursors
- every WindowContext has a non-owned wl_buffer* that holds the cursor contents for its surface
- WaylandMouseContext has a wl_surface* that is always the cursor surface

Backend-specifi - Winapi
------------------------

About winapi, keycodes and unicode:
Winapi makes it very hard for ny to correctly convert keycodes to utf8 string.
To correctly implement the KeyboardContext::utf8 function, WinapiKeyboardContext holds
an internal table of keycodes mapped to their default unicode strings since querying this
when needed might interfer with the real key events.
The only free function for converting keycodes to unicode is ::ToUnicode which does not really
meet all needs.

Some posts regarding clearing the keyboard buffer:
web.archive.org/web/20101004154432/http://blogs.msdn.com/b/michkap/archive/2006/04/06/569632.aspx
web.archive.org/web/20100820152419/http://blogs.msdn.com/b/michkap/archive/2007/10/27/5717859.aspx
