current:
- copyright in header/source (see appContext.hpp) !important
- normalize wheel input values !important
- rework surface (should be WindowContext member) !important
	- other draw integrations good that way?
- rework glx/egl/wgl api loading (glad without loader)
	- make egl loading totally dynamic, so it can e.g. be used on windows with ANGLE
- event type register, see doc/concepts/events
- rework events (concepts/events.md, concepts/window.md), eventDispatcher async funcions
- xkbcommon, unix header error on wrong config (like with gl or backends)
- general keydown/keyup unicode value specificiation (cross-platform, differents atm)

for later:
- touch support (TouchContext and touch events)
- skia integration (would be a great benefit since it might use persistent surfaces and gl/vulkan)

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

winapi backend:
- wgl api reparse [loader, swap control tear]
- better (correct) beginResize/beginMove implementations
- initial mouse focus (see KeyboardContext handler inconsistency)
