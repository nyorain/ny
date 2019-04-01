# Todo

Many points of the stuff below are not actual bugs and rather ideas
for features/improvements.

## Prio

- windows: wm_sysdown to capture alt keypress
- windows: wm_char fix (test with ime)
- test correct/uniform wheel values on all platforms (1.f per tick)
- rework dataExchange
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
	- dataExchange: use std::variant instead of std::any
		- the possible types are known
		- further are custom types really bad
	- DataOffer: methods const? they do not change the state of the object (interface)
		- may not be threadsafe in implementation; should not be required (should it?)
		- also: really pass it as unique ptr in WindowListener::drop
			- why not simply non-const pointer, from this can be moved as well?!
	- wayland: fix dataExchange, enable only for droppable windows
		- probably -> dataExchange rework

### missing features/design issues

- dynamic casts for EventData bad design
	- rather guarantee that a given backend always uses a fix type and
	  make the user check on backend (in ny::Backend)
	- also bad design for WindowContext creation? probably makes more
	  sense to use some extension struct pointer on WindowSettings
	  that MUST be set to the right (backend-specific) type
- AppSettings
	- for android: make the log name dependent on it
		- really useful when multiple ny application running
	- esp. useful wayland/x11 for app name
	- use polymorphism as with WindowSettings, allow backend specific stuff
- create (and cleanup existing) docs
	- document when which events are sent
		- test it on all backends, such things may be possible with half-automated tests
	- make sure spec/promises are kept
		- no event dispatching to handlers outside poll/waitEvents
		- fix initial state/size sending
		- wayland/x11: top level states (init + on change)
		- invalid behaviour with maximize (not supported) on e.g. i3 (basic example, thinks it is maximized but it is not)
- send CloseEvent when WindowContext::close called? define such things somewhere!
- ime input (see winapi backend)
	- use wm char/wm ime char and such, don't use ToUnicode
- improvements to meson android handling
	- e.g. offer way to automate building (at least some scripts)
		- copy libraries using meson (+ script), automatically generate apk
	- add unit tests where possible
- general way to query (sync/async?) size from windowContext?
- x11 dnd window
- improve WindowCapabilities handling for backends
	- not static on x11/wayland regarding decorations, position, sizeLimits
- improve error specifications everywhere
- move deferred.hpp to nytl
- abolish WindowContextPtr, AppContextPtr
- dataExchange: correctly handle utf-8 mimetype
	- only pass utf-8 to the application
- Keyboard/MouseContext use Event in callback?
- fix/re-add examples
	- see src/examples/old
		- provide vulkan example (basic)
		- provide software rendering example
			- show how the handle the returned mutable image
		- cairo/skia examples
- better WindowSettings surface/draw namings
	- WindowSettings::buffer is a rather bad/unintuitive name
- possibility to create popups on wayland per common interface
	- the interface could also create popups on windows (?)
	- something like 'popupParentWindowContext' in windowSettings?
		- problem: on wayland e.g. we might need the parent surface role
- fix/test meson build configs

### ideas/additions, for later

- add android ci (ndk)
- android: fix/improve input
	- (-> Touch support), window coords and input coords don't match currently
- animated cursor support (low prio)
- WindowContext framecallback events? Could be useful on android/wayland
	- shows the application when it can render again. Would be even really useful
		for games. Can this be achieved on windows (it pretty sure cannot on x11)
- add WindowContext capability 'surfaceEvent' to signal it might send surface destroyed events?
- WindowContext destroy event? Called in the listener when the windowContext exists no more?
	- rework destruction/closing of windows?
- C++17 update:
	- use extended aggregate initialization for Event class
		- update for backends (not create explicit structs before calling listener)
- fix/clean up TODO marks in code
- bufferSurface: dirtyBounds parameter for buffer function
- glsurface::apply: dirtyBound parameter (?) + extension using
- save (and provide somewhere) the LoopControl idiom
	- see (last) commit 057bb554339957984f9e593895f2954ffdda7093
- support for multiple seats (mainly wayland)
- new image formats, such as hsl, yuv since they might be supported by some backends?
	- (mainly wayland)
- MouseContext callbacks delta value might go crazy when changing over (mouseCross)
	- reorder <Mouse/Keyobard>Context callback parameters/use Event structs as well
		give them a similiar signature to the WindowListener callbacks

- implement NonOwnedData optimization (less buffer-copies) [dataExchange rework]
	- for offer/source/both?
```cpp
using NonOwnedData = std::variant<
	std::monostate,
	std::string_view,
	Image,
	nytl::Span<std::string>, // or nytl::Span<std::string_view>?
	nytl::Span<std::byte>>;
```

- test image and uri serialize/deserialize
- AppContext: function for ringing the systems bell (at least x11, winapi)
	- contra: available pretty much only on those platforms
- default windowContext surface to use bufferSurface?
	- defaults window tear or show undefined content (bad, not sane default?)
	- most applications want to draw in some way
	- also default clear the buffer in some way? or set a flag for this with
		default set to true?

low prio, for later:
====================

- some common util file/dir for e.g. ConnectionList
- toy around with additional backends e.g. drm
	- mainly to make sure it is possible with the current abstraction
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

- x11: don't use XInitThreads and send_event for synchronization, but
  poll with eventfd like wayland app context
  	- or at least offer settings in something like AppContextSettings
- selections and xdnd improvements (see x11/dataExchange header/source TODO)
	- X11DataSource constructor: check for uri list one file -> filename target format
	- dnd image window
	- https://git.blender.org/gitweb/gitweb.cgi/blender.git/blob/HEAD:/extern/xdnd/xdnd.c
- correct error handling (for xlib calls e.g. glx use an error handle in X11AppContext or util)
	- glx: don't log every error but instead only output error list on total failure?
- KeyboardContext: correct xkb keymap recreation/ event handling
- send correct StateEvents (check for change in configure events?)
- customDecorated: query current de/window manager to guess if they support motif
	- any other (better) way to query this?
- glx screen number fix (don't assume it, store the value from the appcontext)
	- overall correct screen handling

wayland backend:
---------------

- support other protocols
	- stable xdg shell (high prio!)
	- xdg-decoration
	- maybe presentation protocol for timing?
- ShmBuffer shm_pool shared (not one per buffer...)
- fix/simplify data exchange. Formats probably not async
	- fix data retrieving/sending (make sure it always works, even if splitted and stuff)
	- when exactly can we close the fd we read from? check for EOF. Don't block!
- handle window hints correctly (at least try somehow)
- correct capabilites
- WaylandErrorCategory add new protocols!
	- concept good idea at all?
- improve xdg shell v6 support (position, better configure events, popups)
	- min/max size (also implement this for other surface roles)
- maybe there can be multiple over/focus surfaces
- multi seat support (needs api addition)
	- rework whole keyboardContext/mouseContext concept for multi seat
	- allow them to disappear (add a signal)

winapi backend:
---------------

- ime input? at least check if it is needed (-> winapi char fix)
	- yep is needed, japanese input does currently not work
	- in a nutshell:　translate message + wm_char/wm_ime_char + set composition window correctly
	- https://msdn.microsoft.com/en-us/library/dd318581(v=vs.85).aspx
- SetCapture/ReleaseCapture on mouse button down/release
	- needed to make sure that mouse button release events are sent even
		outside the window (is required for usual button press/release handling e.g.)
- initial mouse focus (see KeyboardContext handler inconsistency)
- better documentation about layered window. Move doc out of the source.
- assure/recheck unicode handling for title, window class name etc.
- dnd/clipboard improvements
	- remove clibboardOffer_ from AppContext
- clean up winapi-dependent data type usage, i.e. assure it works for 32 bit and
	potential future typedef changes
- com: correct refadd/release? check with destructor log
- WC: cursor and icon: respect/handle/take care of system metrics
- rethink WinapiWindowContext::cursor implementation.
- Set a cursor when moving the window (beginMove)? windows 10 does not do it

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
- make activity.hpp private header (?)
	- might be useful to some applications though (really?)
	- really error prone to use, should not be from interest for the application
		- application can simply use AppContext
- toggle fullscreen per window flags (?)
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
	- make more stuff public/accessible instead of friend decls
- activity: join thread?
- handle activity pause/resume/stop
