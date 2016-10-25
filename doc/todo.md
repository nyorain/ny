current:
- rework glx/egl/wgl api loading (glad without loader), make it possible to load e.g. ANGLE
- event type register, see doc/concepts/events
- make WindowContext NOT an EventHandler (instead use own backend-specific functions)
- normalize wheel input values

for later:
- touch support (TouchContext and touch events)
- skia integration (would be a great benefit since it might use persistent surfaces and gl/vulkan)
- rethink file/lib structure. Either merge base/backend or fix/recreate app

when reworking gl:
- example egl/wayland: is EglContextGuard really needed? WaylandEglDisplay sufficient?
- best implementation atm: winapi
	- make egl loading totally dynamic, so it can e.g. be used on windows with ANGLE


x11 backend:
- selections and stuff not working at all
- wheel input

wayland backend:
- wheel input
- animated cursor (low prio)
- egl resize events (make DrawIntegration?)
- build/linux2 (wayland not working when built without egl/gl, invalid unique_ptr)
	- implement like x11 with pimpl

winapi backend:
- wgl setup pixel format
