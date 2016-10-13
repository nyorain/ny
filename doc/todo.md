current:
- fix gl for x11
- replace ugly glad wgl/glx/egl apis. Just load the few (like 2 or 3) needed extensions funcs self
	- discuss this, might not make sense
- event type register, see doc/concepts/events
- fix/rework winapi KeyboardContext implementation
- make WindowContext NOT an EventHandler (instead use own backend-specific functions)

for later:
- explicit owned gl context creation on windowContext (esp. multithreading, see main.md)
- touch support (TouchContext and touch events)
- skia integration (would be a great benefit since it might use persistent surfaces and gl/vulkan)
- vulkan extensions from appContext.cpp files to vulkan.cpp files to not include vulkan.h there

when reworking gl:
- example egl/wayland: is EglContextGuard really needed? WaylandEglDisplay sufficient?

wayland backend:
- DrawIntegrations WC::shown_ impl
- animated cursor (low prio)
