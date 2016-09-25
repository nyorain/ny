general:

- close/destroy events (esp. important on winapi backend)
- implement backends
	- x11 backend
		- implement WC
		- extra fcuntionality, decoration
		- glx context/window implementation
		- multithreading has some problems
		- cairo double buffers?
		- egl (low priority)
	- wayland backend (total rework needed to make it work)
	- winapi backend
		- implement all WC interface functions
		- extra windows functionality (e.g. frame drawing)
	- osx (low priority at the moment, osx dev needed :( )
	- mir (low priority)
- window layouts
	- anchors
	- boxes
- more gui widgets with correct implementation
- full opengl implementation
	- legacy opengl needed
	- some optional extension using (?)
- vulkan drawing & context support (using vpp as utility backend in acceptable)
	- adding scene classes infrastructure for better gl/vulkan drawing
- some different style modules/engines (low priority)






current:

- fix gl api and glContext
- surface integration
