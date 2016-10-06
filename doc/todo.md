current:
- fix gl for x11
- replace ugly glad wgl/glx/egl apis. Just load the few (like 2 or 3) needed extensions funcs self
	- discuss this, might not make sense
- event type register, see doc/concepts/events
- xkb keysyms using a xkb map parameter and xkbcommon-keysyms.h
	- some kind of key enum rework needed...

for later:
- touch support (TouchContext and touch events)
- skia integration (would be a great benefit since it might use persistent surfaces and gl/vulkan)
- vulkan extensions from appContext.cpp files to vulkan.cpp files to not include vulkan.h there

when reworking gl:
- example egl/wayland: is EglContextGuard really needed? WaylandEglDisplay sufficient?
