current:
- fix gl for x11
- replace ugly glad wgl/glx/egl apis. Just load the few (like 2 or 3) needed extensions funcs self
	- discuss this, might not make sense
- event type register, see doc/concepts/events
- xkb keysyms using a xkb map parameter and xkbcommon-keysyms.h

for later:
- touch
- skia integration (would be a great benefit since it might use persistent surfaces and gl/vulkan)
