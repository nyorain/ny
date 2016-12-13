Current todo:
=============

- new image formats, such as hsl, yuv since they might be supported by some backends?
- clean up the namespace/prefix mess (e.g. wayland::EventData vs WaylandEventData)
	- should be uniform across backends
- deferred events (i.e. DONT dispatch outside dispatch functions)
	- winapi may do this at the moment (bad!)
	- wayland e.g. may send a draw event from WindowContext::refesh. ok?
	- how complex is it to implement general event deferring? for all backends?
		common implementation? (-> see common util file)
- some fixes/rethinking to AsyncRequest
	- dispatchLoop error handling? exception safety? implement in source!
- better WindowListener functions
	- use events structures!
	- keyboard modifier, state
	- mouse move delta
	- time
- MouseContext callbacks delta value might go crazy when changing over (mouseCross)
	- reorder <Mouse/Keyobard>Context callback parameters/use Event structs as well
		give them a similiar signature to the WindowListener callbacks
- some common util file/dir for e.g. ConnectionList and LoopInterfaceGuard
	(both not really public include where they are atm, both not really src)
- nytl:
	- SizedStringParam constructor from StringParam
	- remove using namespace nytl from fwd
		- rethink general nytl use (especially CompFunc/Callback)
- dataExchange: make whole usage optional with WindowContext windowflags?
	- e.g. winapi: DropTarget is always register atm.
- normalize wheel input values in some way across backends
- general keydown/keyup unicode value specificiation (cross-platform, differents atm)
	- which event should contain the utf8 member set?
- AppContext settings
	- esp. useful wayland/x11 for app name
- default windowContext surface to use bufferSurface?
	- defaults window tear or show undefined content
	- most applications want to draw in some way
	- also default clear the buffer in some way? or set a flag for this with
		default set to true?
- use pass-by-value for nytl::Vec2 params, everywhere
- test image and uri serialize/deserialize
- popups and dialogs -> different window types (especially modal ones!)
- some kind of dnd offer succesful feedback
	- also some kind of feedback for dataSources on whether another application received it?
	- which format was chosen in the end? none?
	- also: dndEnter event really needed? just send dndMove to introduce it?
- automatic DataFormat conversion
	- more mime-specific DataFormats, i.e. represent "text/XXX" mime-types always as string?
	- e.g. if format uriList is available, it can also always be seen as text
	- does this make sense?
- implement the "less event processing optimization" (-> see deferred events)
	- on all backends (where possible): first process all available events, then send them.
	- prevents that e.g. a size event is sent although the next size event is already known
	- some general event dispatching utiliy helpers for AppContext implementations?
- AppContext: function for ringing the system bell
- testing! add general tests for all features
- documentation
	- operation
	- dataExchange fix

low prio, for later:
====================

- touch support (TouchContext and touch events)
- BufferSurfaceSettings, things like preferred strideAlign or format
	- double buffers setting
	- cairo/skia integration mockups
	- rewrite/fix cairo/skia examples
- noexcept specifications (especially for interfaces!)
- rework glx/egl/wgl api loading (glad without loader)
	- make egl loading totally dynamic, so it can e.g. be used on windows with ANGLE
	- better gl proc addr loading
		- i.e. use gl/gles Library members correctly
- mir, osx, android support

Backend stuff
=============

x11 backend:
------------

- selections and xdnd
- correct error handling (for xlib calls e.g. glx use an error handle in X11AppContext or util)
	- glx: don't log every error but instead only output error list on total failure?
- beginResize/beginMove bug
- icccm: follow ping protocol/set pid (set application class)
- windowsettings init toplevel states
- handle window hints correctly (customDecorated!)
- egl support instead of glx?
- KeyboardContext: correct xkb keymap recreation/ event handlign

wayland backend:
---------------

- animated cursor (low prio)
- support xdg popup (and version 6), other protocols (low prio)
- ShmBuffer shm_pool shared (not one per buffer...)
- handle window hints correctly (at least try somehow)
- correct capabilites
- WaylandErrorCategory new protocols!
- improve xdg shell v6 support (position, better configure events, popups)
	- min/max size (also implement this for other surface roles)

winapi backend:
---------------

- winapi bufferSurface pre-allocate bigger buffer (lien l30)
- ime input? at least check if it is needed
- assure/recheck unicode handling for title, window class name etc.
- native widgets (for all backends relevant)
- wgl api reparse [loader, swap control tear]
- rethink WinapiWindowContext::cursor implementation.
- Set a cursor when moving the window (beginMove)? windows 10 does not do it
- initial mouse focus (see KeyboardContext handler inconsistency)
- better documentation about layered window, make it optional. Move it out of the source.
- better dnd/clipboard
	- think about WM_CLIPBOARDUPDATE
	- remove clibboardOffer_ from AppContext
	- startDragDrop without blocking
		- ability to cancel it
- windowsettings init toplevel states
- clean up winapi-dependent data type usage, i.e. assure it works for 32 bit and
	potential future typedef changes
- does it really make sense to store the WindowContext as user window longptr?
	- cannot differentiate to windows created not by ny
- com: correct refadd/release? check with destructor log!
- com: only need one DropSource/Target helper?
- WC: cursor and icon: respect/handle/take care of system metrics
