- fix gl & vulkan & surface integration for x11/wayland
	- draw integration rework, see WinapiWindowContext
- replace ugly glad wgl/glx/egl apis. Just load the few (like 2 or 3) needed extensions funcs self
	- with that, remove the external dir
- event type register, see doc/concepts/events

for later:

- touch
