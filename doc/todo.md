current:
- combine pls
	- copyright in header/source (see appContext.hpp) !important
	- abolish ny/include.hpp (use fwd where needed and config where needed)
- normalize wheel input values in some way !important
- implement drawing (surfaces/abolish cairo n stuff) like in docs
- BufferSurfaceSettings, things like preferred strideAlign or format
- noexcept specifications (especially for interfaces!)
- rework glx/egl/wgl api loading (glad without loader)
	- make egl loading totally dynamic, so it can e.g. be used on windows with ANGLE
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
- beginResize/beginMove bug
- icccm: follow ping protocol/set pid

wayland backend:
- wglSetup like glx/egl (with init function)
- animated cursor (low prio)
- support xdg popup (and version 6), other protocols (low prio)
- ShmBuffer shm_pool shared (not one per buffer...)
- remove wayland/interfaces (instead use direct object callbacks for window/app)

winapi backend:
- wgl api reparse [loader, swap control tear]
- better (correct) beginResize/beginMove implementations
- initial mouse focus (see KeyboardContext handler inconsistency)
