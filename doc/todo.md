current:

- update backends for new LoopControl
- rework data (see dataExchange.md)
	- async request, event dispatching (nested?)

- normalize wheel input values in some way across backends
- event type register, see doc/concepts/events
- rework events (concepts/events.md, concepts/window.md), eventDispatcher async funcions
- general keydown/keyup unicode value specificiation (cross-platform, differents atm)
	- which event should contain the utf8 member set?
- AppContext settings
- default windowContext surface to bufferSurface?
- remove using namespace nytl from fwd
- use pass-by-value for nytl::Vec2 params
- touch support (TouchContext and touch events)
- test image and uri serialize/deserialize

low prio, for later:

- BufferSurfaceSettings, things like preferred strideAlign or format
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
- remove wayland/interfaces (instead use direct object callbacks for window)

winapi backend:
- wgl api reparse [loader, swap control tear]
- better (correct) beginResize/beginMove implementations (try at least?)
- initial mouse focus (see KeyboardContext handler inconsistency)
- better dnd
- windowsettings init toplevel states
