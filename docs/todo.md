# Todo

Many points of the stuff below are not actual bugs and rather ideas
for features/improvements.

## Prio

- WindowContext::refresh doesn't work this way, not even on wayland
  rather call it something like *frameCallback* and require that
  it's called *prior* to (finishing) drawing
  	- we can't know about e.g. vulkan presents
- there are some serious issues with the whole deferred thing
  recheck (and test!) implementations to make sure that AppContexts
  (especially wayland,x11) never block when there are events/deferred
  handlers left.
- fix cairo example with constant refresh...
	- doesn't work e.g. on x11 backend
	- also use present extension on x11 buffer surface or get rid of it
- update backends to respect WindowSettings::customDecorated
  (done for x11, wayland; needs to be done for winapi)
- fix aliasing issues for x11 backend. Don't reinterpret_cast
  (probably best to just revisit all reinterpret_casts)
  	- also fix them for all vulkan surfaces
- make AppContext::deferred private everywhere
	- instead add `defer(func)` and `windowContextDestroyed(wc)` functions
- remove the keyboard and mouse context callbacks. Not needed,
  often not correctly implemented/called
- make code reentrant, fix next_ in X11AppContext
	- but also check other appcontexts again
- cleanup and write documentation
- ny::Surface: use std::variant
	- there are probably other unions in ny as well

- windows: wm_sysdown to capture alt keypress
- windows: wm_char fix (test with ime), document it
- test correct/uniform wheel values on all platforms (1.f per tick)
	- currently inverted on wayland...
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
	- DataOffer: methods const? they do not change the state of the object (interface)
		- may not be threadsafe in implementation; should not be required (should it?)
		- also: really pass it as unique ptr in WindowListener::drop
			- why not simply non-const pointer, from this can be moved as well?!
	- wayland: fix dataExchange, enable only for droppable windows
		- probably -> dataExchange rework
- better WindowSettings surface/draw namings
	- WindowSettings::buffer is a rather bad/unintuitive name

### missing features/design issues

- animated cursors
	- in Cursor class (low prio) but we should at least support animated system cursors
	  e.g. on wayland
- allow hotspot/offset for dnd windows (in data source?)
- dynamic casts for EventData bad design?
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
- document when which events are called, guarantees
	e.g. no resize events on manual resize, no closeEvent on close
	but initial resize event if defaultSize was passed in WindowSettings
- improvements to meson android handling
	- e.g. offer way to automate building (at least some scripts)
		- copy libraries using meson (+ script), automatically generate apk
	- add unit tests where possible
- add general way to query (sync/async?) size from windowContext?
- improve error specifications everywhere
- move deferred.hpp to nytl
- dataExchange: correctly handle utf-8 mimetype
	- only pass utf-8 to the application
- fix/re-add examples
	- see src/examples/old
		- provide vulkan example (basic)
		- provide software rendering example
			- show how the handle the returned mutable image
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
- better (tested) touch support
	- maybe offer settings to just translate them into pointer evens?
	- implement touch support on winapi, wayland

### ideas/additions, for later; things that need discussion

- is there a need for a `TouchContext` class (like Keyboard/MouseContext?)
- allow to mark WindowContext "urgent"
	- not sure yet how it's implemented in x11,wayland but there is a common
	  mechanism that quite some apps use. Probably something like this in winapi
	  as well
- DndAction is currently intersection of backends. We could make it
  union of backends (adding ask/link/private)
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
- bufferSurface: dirtyBounds parameter for buffer function
	- can be handled e.g. on wayland
- glsurface::apply: dirtyBound parameter (?) + extension using
- support for multiple seats (mainly wayland)
- [extremely low prio, probably bad idea] possibility to create popups/dialogs/subsurfaces
	- the interface could also create popups on windows (?)
	- something like 'popupParentWindowContext' in windowSettings?
	  what about subsurfaces?
- new image formats, such as hsl, yuv since they might be supported by some backends?
	- (mainly wayland)
- test image and uri serialize/deserialize
- AppContext: function for ringing the systems bell (at least x11, winapi)
	- contra: available pretty much only on those platforms
- default windowContext surface to use bufferSurface?
	- defaults window tear or show undefined content (bad, not sane default?)
	- most applications want to draw in some way
	- also default clear the buffer in some way? or set a flag for this with
		default set to true?
- toy around with additional backends e.g. drm
	- mainly to make sure it is possible with the current abstraction
- BufferSurfaceSettings, things like preferred strideAlign or format
	- double buffering setting
- BufferSurface: skia integration example
- noexcept specifications (especially for interfaces!)
- osx support

Backend stuff
=============

x11 backend:
------------

- replace x11 ErrorCatgery checkwarn with a macro that preserved original location in code?
- most of the current xcb_flush calls are probably not needed, bad for performance
- selections and xdnd improvements (see x11/dataExchange header/source TODO)
	- X11DataSource constructor: check for uri list one file -> filename target format
- correct error handling (for xlib calls e.g. glx use an error handle in X11AppContext or util)
	- glx: don't log every error but instead only output error list on total failure?
- KeyboardContext: correct xkb keymap recreation/ event handling
- send correct StateEvents (check for change in configure events?)
- customDecorated: query current de/window manager to guess if they support motif
	- any other (better) way to query this?
- overall correct screen (and screen number) handling

wayland backend:
---------------

- window state events sometimes send wrong
  test out with examples/basic, it sometimes thinks its minimized/maximized
  even though it isn't
- allow client side to set window geometry (especially when custom decorated)
- support other protocols
	- xdg-decoration
	- maybe presentation protocol for timing?
- performance: ShmBuffer shm_pool shared (not one per buffer...)
- check again whether custom show/hide implementation is possible
	- or re-evaluate whether show/hide is even really needed
	  what would be a reasonable use case to suddenly hide/show a window?!
- WaylandErrorCategory: add new protocols!
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
