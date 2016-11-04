current:
- rework glx/egl/wgl api loading (glad without loader)
	- make egl loading totally dynamic, so it can e.g. be used on windows with ANGLE
- event type register, see doc/concepts/events
- normalize wheel input values
- rework events (concepts/events.md, concepts/window.md), eventDispatcher async funcions
- xkbcommon, unix header error on wrong config (like with gl or backends)
- general keydown/keyup unicode value specificiation (cross-platform, differents atm)

for later:
- touch support (TouchContext and touch events)
- skia integration (would be a great benefit since it might use persistent surfaces and gl/vulkan)

x11 backend:
- selections and stuff not working at all
- wheel input
- correct error handling (for xlib calls e.g. glx use an error handle in X11AppContext or util)

wayland backend:
- wheel input
- animated cursor (low prio)

winapi backend:
- wgl api reparse [loader, swap control tear]
- better beginResize/beginMove implementations
