current:
- rework glx/egl/wgl api loading (glad without loader), make it possible to load e.g. ANGLE
- event type register, see doc/concepts/events
- make WindowContext NOT an EventHandler (instead use own backend-specific functions)

for later:
- explicit owned gl context creation on windowContext (esp. multithreading, see main.md)
- touch support (TouchContext and touch events)
- skia integration (would be a great benefit since it might use persistent surfaces and gl/vulkan)
- vulkan extensions from appContext.cpp files to vulkan.cpp files to not include vulkan.h there

when reworking gl:
- example egl/wayland: is EglContextGuard really needed? WaylandEglDisplay sufficient?

wayland backend:
- animated cursor (low prio)
- egl resize events (make DrawIntegration?)
- build/linux2 (wayland not working when built without egl/gl, invalid unique_ptr)
	- implement like x11 with pimpl

winapi backend:
- correct gl context management. (best example atm: x11 backend)
- fix/rework winapi KeyboardContext/MouseContext implementation
	- they .e.g. dont call the callbacks
	- they should manage sent events
