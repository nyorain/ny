current:

- update backends for new LoopControl !important
- do combined: !important
	- copyright in header/source (see appContext.hpp)
	- abolish ny/include.hpp (use fwd where needed and config where needed)
- rework data !important (see dataExchange.md)

- normalize wheel input values in some way !important
- implement drawing (surfaces/abolish cairo n stuff) like in docs
- BufferSurfaceSettings, things like preferred strideAlign or format
	- cairo/skia integration mockups
- noexcept specifications (especially for interfaces!)
- rework glx/egl/wgl api loading (glad without loader)
	- make egl loading totally dynamic, so it can e.g. be used on windows with ANGLE
	- better gl proc addr loading
		- i.e. use gl/gles Library members correctly
- event type register, see doc/concepts/events
- rework events (concepts/events.md, concepts/window.md), eventDispatcher async funcions
- xkbcommon, unix header #error on when #ifndef NY_WithXkbcommon (need this variable)
- general keydown/keyup unicode value specificiation (cross-platform, differents atm)
	- which event should contain the utf8 member set?
- AppContext settings
- rewrite/fix cairo/skia examples
- default windowContext surface to bufferSurface?
- GlConfigId -> GlConfigID

for later:
- touch support (TouchContext and touch events)
- x11 egl

x11 backend:
- selections and xdnd
- correct error handling (for xlib calls e.g. glx use an error handle in X11AppContext or util)
	- glx: dont' log every error but instead only output error list on total failure?
- beginResize/beginMove bug
- icccm: follow ping protocol/set pid

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
