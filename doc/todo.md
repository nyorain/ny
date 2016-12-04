current:

- update backends for new LoopControl
- update backends for new data
- ny::DataFormat comparsion (should there be a "==" operator?)
- some fixes/rethinking to AsyncRequest
	- dispatchLoop error handling? exception safety? implement in source!

- MouseContext callbacks delta value might go crazy when changing over (mouseCross)

- some common util file/dir for e.g. ConnectionList and LoopInterfaceGuard
	(both not really public include where they are atm, both not really src)

- nytl: SizedStringParam constructor from StringParam

- dataExchange: make whole usage optional with WindowContext windowflags?
	- e.g. winapi: DropTarget is always register atm.

- normalize wheel input values in some way across backends
- event type register, see doc/concepts/events
- rework events (concepts/events.md, concepts/window.md), eventDispatcher async funcions
- general keydown/keyup unicode value specificiation (cross-platform, differents atm)
	- which event should contain the utf8 member set?
- AppContext settings
	- esp. useful wayland/x11 for app name
- default windowContext surface to bufferSurface?
- remove using namespace nytl from fwd
	- rethink general nytl use
- use pass-by-value for nytl::Vec2 params
- test image and uri serialize/deserialize
- popups and dialogs -> different window types (especially modal ones!)
- some kind of dnd offer succesful feedback
	- also some kind of feedback for dataSources on whether another application received it?
	- which format was chosen in the end? none?
	- also: dndEnter event really needed? just send dndMove to introduce it?

- automatic Dataformat conversion
	- more mime-specific DataFormats, i.e. represent "text/XXX" mime-types always as string?
	- e.g. if format uriList is available, it can also always be seen as text
	- does this make sense?

- implement the "less event processing optimization"
	- on all backends (where possible): first process all available events, then send them.
	- prevents that e.g. a size event is sent although the next size event is already known
	- some general event dispatching utiliy helpers for AppContext implementations?

- AppContext: function for ringing the system bell
- keyboard modifier in KeyboardContext or KeyEvent

- documentation
	- operation
	- dataExchange fix

low prio, for later:

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

x11 backend:
- selections and xdnd
- correct error handling (for xlib calls e.g. glx use an error handle in X11AppContext or util)
	- glx: don't log every error but instead only output error list on total failure?
- beginResize/beginMove bug
- icccm: follow ping protocol/set pid (set application class)
- windowsettings init toplevel states
- split event handling (e.g. handle pointer/keyboard events in input.cpp)
- egl support instead of glx?

wayland backend:
- animated cursor (low prio)
- support xdg popup (and version 6), other protocols (low prio)
- ShmBuffer shm_pool shared (not one per buffer...)
- improve xdg shell v6 support (position, better configure events, popups)
	- min/max size (also implement this for other surface roles)

winapi backend:
- winapi bufferSurface pre-allocate bigger buffer (l30)
- unicode handling for title, window class name etc.
- native widgets (for all backends relevant)
- wgl api reparse [loader, swap control tear]
- better (correct) beginResize/beginMove implementations (try at least?)
- initial mouse focus (see KeyboardContext handler inconsistency)
- cursor handling impl
	- general
	- especially for custom move/resize
- better dnd/clipboard
	- think about WM_CLIPBOARDUPDATE
	- remove clibboardOffer_ from AppContext
	- startDragDrop without blocking
- windowsettings init toplevel states
- clean up winapi-dependent data type usage, i.e. assure it works for 32 bit and
	potential future typedef changes
