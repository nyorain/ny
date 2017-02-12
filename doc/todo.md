# Needed fixes:

### priority

- SurfaceEvent[s] that e.g. signal recreated surfaces
	- SurfaceDestroy & SurfaceCreate events?
	- see the android egl/vulkan design problem for window recreation
- WindowListener::surfaceDestroyed: output warning on default implementation
	- it was not overriden which can/will lead to serious problems.
- fix loopControl [synchronization] [pretty much done, fix for backends]
	- make sure that impl isnt changed during operation on it?!
		- mutex in loopControl?
			- use shared mutex (C++17)
		- use shared ptr?
	- make LoopInterface class movable (and default ctor protected)
		- useful for late initialization
			- needed?
	- testing; code review!
- egl/wgl/glx library loading (dynamically load opengl)
	- should GlSetup::procAddr be able to query gl core functions?
	- fix egl (check in context creation if extension/egl 1.5 available)
	- apientryp needed for pointer declarations?
	- does the force version flag really makes sense?
		- version setting at all? are there some (real) cases where a
			specific opengl version is needed?
	- fix egl/wgl error handling
		- GlContextErrc::contextNotCurrent (e.g. swapInterval)
	- query gl context version as attribute
- fix examples
	- see src/examples/old
		- provide vulkan example (basic)
		- provide software rendering example
			- show how the handle the returned mutable image
		- cairo/skia
- better WindowSettings surface/draw namings
	- WindowSettings::buffer is a rather bad/unintuitive name
- make position vecs (e.g. for mouseButton, mouseMove) Vec2ui instead of Vec2i
	- when could it be negative?
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
	- also: AsyncRequest wait return type? the bool return type is rather bad.
		perfer excpetions in AppContext for critical errors? would really make more sense...
		the function might already throw if any listener/callback throw so it should
		throw on critical errors!
	- or is it ok this way? applications ususually can't do much about critical errors...
		and to modify where ny logs it, they can just change the log streams
- fix building config:
	- which cmake version is needed?
	- see: gl linkin/building
	- better find xcb cmake script
		- make sure icccm and ewmh are found (are they available everywhere?)
	- test wayland config
		- wayland without egl, wayland without cursor library
- abolish WindowContextPtr, AppContextPtr
	- names like UniqueWindowContext more acceptable (?)
- dataExchange: use std::variant instead of std::any
	- the possible types are known
	- further are custom types really bad

### later; general; rework needed

- easier image rework? (old format impl?)
	- then fix android bufferSurface format query (dont assume rgba8888)
- C++17 update:
	- use extended aggregate initialization for Event class
		- update for backends (not create explicit structs before calling listener)
- fix/clean up TODO marks in code
- bufferSurface: dirtyBounds parameter for buffer function
- glsurface::apply: dirtyBound parameter
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

further improvements:
=============

- testing! add general tests for all features
	- test image functions on big endian machine
- documentation
	- fix/remove/split main.md
	- operation
	- dataExchange fix

low prio, for later:
====================

- some common util file/dir for e.g. ConnectionList and LoopInterfaceGuard
	(both not really public include where they are atm, both not really src)
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
- mir, osx support

Backend stuff
=============

x11 backend:
------------

- selections and xdnd improvements (see x11/dataExchange header/source TODO)
	- X11DataSource constructor: check for uri list one file -> filename target format
	- dnd image window
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

wayland backend:
---------------

- animated cursor (low prio)
- support xdg popup (and version 6), other protocols (low prio)
- ShmBuffer shm_pool shared (not one per buffer...)
- handle window hints correctly (at least try somehow)
- correct capabilites
- WaylandErrorCategory new protocols!
- improve xdg shell v6 support (position, better configure events, popups)
	- min/max size (also implement this for other surface roles)

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

- android/activity: error handling for unexpected situations
	- e.g. multiple windows/queue
	- invalid native activities
		- at least output warning in retrieveActivity?
	- there are (theoretically) a few threadunsafe calls
		in Activity (e.g. the appContext checks)
		- rather use mutex for synchro
- call the main thread function from a c function (compiled with a c compiler)
	because calling main in c++ is not allowed (it is in C)
- correct window recreation
	- make WindowContext, AndroidEglSurface, ... recreate themselves
- make sure callbacks function can NEVER throw
- better main thread throw handling
	- application just closes without anything atm (can it be done better?)
- AndroidWindowSettings (for buffer surface)
	- give the possibility to choose format (rgba, rgbx, rgb565)
- make activity.hpp private header
	- really error prone to use, should not be from interest for the application
		- application can simply use AppContext
- toggle fullscreen per window flags (?!)
- AppContext: expose keyboard show/hide functions
- android:egl:
	- make sure to apply the egl config format to the native window
