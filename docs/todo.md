# Needed fixes:

- after some cleanups (+important fixes): release first alpha (clear roadmap?)
	- think about api/abi guarantees to give
	- improve docs/make gen doc pages?

### bug fixes, important

- fix android
	- add meson support, port own apk.cmake
	- move the cpp examples out of android folder
		- use mainlined examples (fix them to work for ALL (android!) platforms)

### missing features/design issues, not so important

- dataExchange: correctly handle utf-8 mimetype
	- only pass utf-8 to the application
- Keyboard/MouseContext use Event in callback
- WindowListener::surfaceDestroyed: output warning on default implementation?
	- it was not overriden which can/will lead to serious problems.

### ideas/additions, for later

- WindowContext framecallback events? Could be useful on android/wayland
	- shows the application when it can render again. Would be even really useful
		for games. Can this be achieved on windows (it pretty sure cannot on x11)
- add WindowContext capability 'surfaceEvent' to signal it might send surface destroyed events?
- WindowContext destroy event? Called in the listener when the windowContext exists no more?
	- rework destruction/closing of windows?

### priority

- fix loopControl [synchronization] [pretty much done, fix for backends maybe needed]
	- make sure that impl isnt changed during operation on it?!
		- mutex in loopControl?
			- use shared mutex (C++17)
		- use shared ptr?
	- make LoopInterface class movable (and default ctor protected)
		- useful for late initialization
			- needed?
	- testing; code review!
- fix android backend
	- integrate with meson
- fix examples
	- see src/examples/old
		- provide vulkan example (basic)
		- provide software rendering example
			- show how the handle the returned mutable image
		- cairo/skia
- better WindowSettings surface/draw namings
	- WindowSettings::buffer is a rather bad/unintuitive name
- fix WindowSettings handling for backends
- send state and size events on windowContext creation? (other important values?)
	- general way to query size from windowContext?
	- query other values? against event flow but currently things like
		toggling fullscreen are hard/partly impossible
- fix WindowCapabilites for backends
	- make return value dependent on child or toplevel window
	- query server caps if possible
- AppContext error handling? Give the application change to retrieve some error (code,
	exception?) when e.g. AppContext::dispatchLoop returns false
	- also: AsyncRequest::wait return type? the bool return type is rather bad.
		perfer excpetions in AppContext for critical errors? would maybe make more sense...
		the function might already throw if any listener/callback throws so it should
		throw on critical errors
	- or is it ok this way? applications ususually can't do much about critical errors...
- fix building config:
	- which meson version is needed?
	- see: gl linkin/building
	- test wayland config
		- wayland without egl, wayland without cursor library
- abolish WindowContextPtr, AppContextPtr
	- names like UniqueWindowContext more acceptable (?)
- dataExchange: use std::variant instead of std::any
	- the possible types are known
	- further are custom types really bad

### later; general; rework needed

- C++17 update:
	- use extended aggregate initialization for Event class
		- update for backends (not create explicit structs before calling listener)
- fix/clean up TODO marks in code
- bufferSurface: dirtyBounds parameter for buffer function
- glsurface::apply: dirtyBound parameter (?)
- DataOffer: methods const? they do not change the state of the object (interface)
	- may not be threadsafe in implementation; should not be required (should it?)
	- also: really pass it as unique ptr in WindowListener::drop
		- why not simply non-const pointer, from this can be moved as well?!
- new image formats, such as hsl, yuv since they might be supported by some backends?
	- needs image format rework
	- (mainly wayland)
- deferred events (i.e. DONT dispatch outside dispatch functions)
	- winapi may do this at the moment (bad!) (does it really? just use async and check!)
	- wayland e.g. may send a draw event from WindowContext::refesh. ok?
	- how complex is it to implement general event deferring? for all backends?
		common implementation? (-> see common util file)
- MouseContext callbacks delta value might go crazy when changing over (mouseCross)
	- reorder <Mouse/Keyobard>Context callback parameters/use Event structs as well
		give them a similiar signature to the WindowListener callbacks
- normalize wheel input values in some way across backends
	- like value 1 if one "tick" was scrolled?
- general keydown/keyup unicode value specificiation (cross-platform, differents atm)
	- which event should contain the utf8 member set?
- AppContext settings
	- for android: make the log name dependent on it
		- really useful when multiple ny application running
	- esp. useful wayland/x11 for app name
	- could also be used for different logger (ny/log.hpp) initializations
		- e.g. something like a --log or --verbose flag for ny itself
		- program arguments passed with AppContextSettings
- default windowContext surface to use bufferSurface?
	- defaults window tear or show undefined content (bad, not sane default?)
	- most applications want to draw in some way
	- also default clear the buffer in some way? or set a flag for this with
		default set to true?
- test image and uri serialize/deserialize
- popups and dialogs -> different window types (especially modal ones!)
- some kind of dnd offer succesful feedback
	- also offer dnd effects (copy/move etc)
	- return something like AsynRequest from AppContext::startDragDrop
	- also some kind of feedback for dataSources on whether another application received it?
	- which format was chosen in the end? none?
	- also: dndEnter event really needed? just send dndMove to introduce it?
- automatic DataFormat conversion
	- more mime-specific DataFormats, i.e. represent "text/XXX" mime-types always as string?
	- e.g. if format uriList is available, it can also always be seen as text
	- does this make sense?
- implement the "less event processing optimization" (-> see deferred events)
	- on all backends (where possible): first process all available events, then send them.
	- prevents that e.g. a size event is sent although the next size event is already known
	- some general event dispatching utiliy helpers for AppContext implementations?
- AppContext: function for ringing the systems bell (at least x11, winapi)
- send CloseEvent when WindowContext::close called? define such things somewhere!
- gl: (re-)implement shared checking functionality
	- only if it possible in a lightweight manner
- egl/wgl/glx library loading (dynamically load opengl)
	- should GlSetup::procAddr be able to query gl core functions?
	- fix egl (check in context creation if extension/egl 1.5 available)
	- apientryp needed for pointer declarations?
	- fix egl/wgl error handling
		- GlContextErrc::contextNotCurrent (e.g. swapInterval)
	- query gl context version as attribute

further improvements:
=============

- testing! add general tests for all features
- documentation
	- fix/remove/split main.md
	- operation
	- dataExchange fix

low prio, for later:
====================

- some common util file/dir for e.g. ConnectionList and LoopInterfaceGuard
	(both not really public include where they are atm, both not really src)
- toy around with additional backends e.g. drm
	- mainly to make sure it is possible with the current abstraction
- LoopControl: callIn implementation (combines timer and function call)
	- can be merged with call function by using `now` as special time value
- touch support (TouchContext and touch events)
	- maybe just translate them into pointer evens?
	- more general events that cover touch and pointer input,
		- they are conceptually the same, some backends make little difference
		- maybe just add more information to pointer events and associated
			them with a pointer id that allows for multiple pointer handling?
- BufferSurfaceSettings, things like preferred strideAlign or format
	- double buffers setting
	- cairo/skia integration mockups
	- rewrite/fix cairo/skia examples
- noexcept specifications (especially for interfaces!)
- osx support
- make position vecs (e.g. for mouseButton, mouseMove) Vec2ui instead of Vec2i
	- when could it be negative?
	- not sure if good idea

Backend stuff
=============

x11 backend:
------------

- fix windowContext operations/implementation
	- customDecorated/beginMove/beginResize
	- https://github.com/nwjs/chromium.src/blob/45886148c94c59f45f14a9dc7b9a60624cfa626a/ui/base/x/x11_util.cc
- rething dependency on xcb-ewmh and xcb-icccm (really makes sense?)
- selections and xdnd improvements (see x11/dataExchange header/source TODO)
	- X11DataSource constructor: check for uri list one file -> filename target format
	- dnd image window
	- https://git.blender.org/gitweb/gitweb.cgi/blender.git/blob/HEAD:/extern/xdnd/xdnd.c
- correct error handling (for xlib calls e.g. glx use an error handle in X11AppContext or util)
	- glx: don't log every error but instead only output error list on total failure?
- beginResize/beginMove bug
- icccm: follow ping protocol/set pid (set application class, use pid protocol)
- windowsettings init toplevel states
- handle window hints correctly (customDecorated!)
- egl support instead of/additionally to glx?
- KeyboardContext: correct xkb keymap recreation/ event handling
- send correct StateEvents (check for change in configure events?)
- customDecorated: query current de/window manager to guess if they support motif
	- any other (better) way to query this?
- glx screen number fix (don't assume it, store the value from the appcontext)

wayland backend:
---------------

- key repeat (probably best to use a timerfd, see weston example clients)
	- then rework key repeat check in input.cpp (can probably be done better)
- animated cursor (low prio)
- support xdg popup (and version 6), other protocols (low prio)
- ShmBuffer shm_pool shared (not one per buffer...)
- fix/simplify data exchange. Formats probably not async
	- fix data retrieving/sending (make sure it always works, even if splitted and stuff)
	- when exactly can we close the fd we read from? check for EOF. Don't block!
- handle window hints correctly (at least try somehow)
- correct capabilites
- WaylandErrorCategory new protocols!
- improve xdg shell v6 support (position, better configure events, popups)
	- min/max size (also implement this for other surface roles)
- maybe there can be multiple over/focus surfaces
- multi seat support (needs api addition)
	- rework whole keyboardContext/mouseContext concept for multi seat
	- allow them to disappear (add a signal)

winapi backend:
---------------

- SetCapture/ReleaseCapture on mouse button down/release
	- needed to make sure that mouse button release events are sent even
		outside the window (is required for usual button press/release handling e.g.)
- assure/recheck unicode handling for title, window class name etc.
- rethink WinapiWindowContext::cursor implementation.
- initial mouse focus (see KeyboardContext handler inconsistency)
- better documentation about layered window, make it optional. Move doc out of the source.
- dnd/clipboard improvements
	- think about WM_CLIPBOARDUPDATE
	- remove clibboardOffer_ from AppContext
	- startDragDrop without blocking
		- ability to cancel it (general design)
- windowsettings init toplevel states
- clean up winapi-dependent data type usage, i.e. assure it works for 32 bit and
	potential future typedef changes
- does it really make sense to store the WindowContext as user window longptr?
	- cannot differentiate to windows created not by ny
- com: correct refadd/release? check with destructor log!
- WC: cursor and icon: respect/handle/take care of system metrics

- Set a cursor when moving the window (beginMove)? windows 10 does not do it
- native widgets rethink (for all backends relevant)
	- at least dialogs are something (-> see window types)
- ime input? at least check if it is needed
- egl backend (see egl.cpp)
	- optional instead of wgl
	- check if available, use wgl instead
	- should be easy to implement
	- might be more efficient (ANGLE) than using wgl

android
=======

- android/activity: more error handling for unexpected situations
	- at least output a warning or something so its debuggable
		- even for situations that indicate android bug
	- e.g. multiple windows/queue
	- invalid native activities
		- at least output warning in retrieveActivity?
	- there are (theoretically) a few threadunsafe calls
		in Activity (e.g. the appContext checks)
		- rather use mutex for synchro
- make sure activity callbacks function can not throw
	- this will kill the process
- better handling of exception out of ::main function
	- application just closes without anything atm (can it be done better?)
- AndroidWindowSettings (for buffer surface)
	- give the possibility to choose format (rgba, rgbx, rgb565)
- make activity.hpp private header ?
	- might be useful to some applications though (really?)
	- really error prone to use, should not be from interest for the application
		- application can simply use AppContext
- toggle fullscreen per window flags (?!)
- possibility to save state/ retrieve saved state
	- don't store them all the time might be much data
	- sth like only store it / make it retrievable until no AppContext was created yet
- activity:redrawNeeded: check if size changed/capture premature refresh
	- see function todos
- fix gl example, android/egl nativeWindow impl
	- make egl surface not current on destruction
		- output warning in nativeWindow but try to recover at least
			- check if same thread or not. What if not?
- AppContext: some call in destructor to release ALooper after ALooper_prepare?
- rethink android::Activity protection
	- the class is already private interface should it really use private members
		and friend declarations? its probably somewhat overdesigned...
- activity: join thread?
- handle activity pause/resume/stop
