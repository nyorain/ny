# Needed fixes:

### priority

- implement windowHints/keyboard modifier changes for x11/wayland
- egl/wgl/glx library loading (dynamically load opengl)
	- link e.g. core egl statically?
- fix examples
	- remove unneeded ones
- fix WindowSettings handling for backends
- fix WindowCapabilites for backends
- AppContext error handling? Give the application change to retrieve some error (code,
	exception?) when e.g. AppContext::dispatchLoop returns false
	- also: AsyncRequest wait return type? the bool return type is rather bad.
		perfer excpetions in AppContext for critical errors? would really make more sense...
		the function might already throw if any listener/callback throw so it should
		throw on critical errors!

### later; general; rework needed

- C++17 update:
	- use extended aggregate initialization for Event class
		- update for backends (not create explicit structs before calling listener)
- fix/clean up TODO marks in code
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
- some common util file/dir for e.g. ConnectionList and LoopInterfaceGuard
	(both not really public include where they are atm, both not really src)
- dataExchange: make whole usage optional with WindowContext windowflags?
	- e.g. winapi: DropTarget is always register atm.
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
- use pass-by-value for nytl::Vec2 params, everywhere
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

- touch support (TouchContext and touch events)
- BufferSurfaceSettings, things like preferred strideAlign or format
	- double buffers setting
	- cairo/skia integration mockups
	- rewrite/fix cairo/skia examples
- noexcept specifications (especially for interfaces!)
- rework glx/egl/wgl api loading (glad without loader)
	- make egl loading totally dynamic, so it can e.g. be used on windows with ANGLE
	- better gl proc addr loading
		- i.e. use gl/gles Library members correctly
- mir, osx, android support

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

- winapi bufferSurface pre-allocate bigger buffer (lien l30)
- egl backend (see egl.cpp)
	- optional instead of wgl
	- check if available, use wgl instead
	- should be easy to implement
	- might be more efficient (ANGLE) than using wgl
- ime input? at least check if it is needed
- assure/recheck unicode handling for title, window class name etc.
- native widgets (for all backends relevant)
- wgl api reparse [loader, swap control tear]
- rethink WinapiWindowContext::cursor implementation.
- Set a cursor when moving the window (beginMove)? windows 10 does not do it
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
