#include 1. subdir cmake

#general
set(H_GENERAL
	config.h
	ny.hpp
	include.hpp
	window.hpp
	frame.hpp
	widget.hpp
	widgets.hpp
	dialog.hpp
	style.hpp
	windowDefs.hpp
	windowEvents.hpp
	drawContext.hpp
	shape.hpp
	color.hpp
	cairo.hpp
	font.hpp
	freeType.hpp
	image.hpp
	texture.hpp
	app.hpp
	cursor.hpp
	data.hpp
	error.hpp
	event.hpp
	eventHandler.hpp
	file.hpp
	keyboard.hpp
	mouse.hpp
	surface.hpp
	backend.hpp
	appContext.hpp
	windowContext.hpp
	)

install(FILES ${H_GENERAL} DESTINATION include/ny)

#utils
set(H_UTIL
	util/transformable.hpp
	util/vec.hpp
	util/mat.hpp
	util/rect.hpp
	util/callback.hpp
	util/thread.hpp
	util/time.hpp
	util/region.hpp
	util/misc.hpp
	util/animation.hpp
	util/refVec.hpp
	util/nonCopyable.hpp
	)

install(FILES ${H_UTIL} DESTINATION include/util)

#specific backends
#gl___________________________________________________________________________________
if(NY_WithGL)
	set(H_GL
	gl/glContext.hpp
	gl/glDC.hpp
	gl/shader.hpp
	)

	install(FILES ${H_GL} DESTINATION include/ny/gl)

endif(NY_WithGL)


#x11___________________________________________________________________________________
if(NY_WithX11)
	set(H_X11
		x11/x11AppContext.hpp
		x11/x11Backend.hpp
		x11/x11Cairo.hpp
		x11/x11Utils.hpp
		x11/x11WindowContext.hpp
		x11/x11Include.hpp
		)

	if(NY_WithGL)
		set(H_X11 ${H_X11}
			x11/x11Egl.hpp
			x11/glx.hpp
			)		
	endif(NY_WithGL)

	install(FILES ${H_X11} DESTINATION include/ny/x11)

endif(NY_WithX11)


#wayland___________________________________________________________________________________
if(NY_WithWayland)
	set(H_WAYLAND
		wayland/waylandAppContext.hpp
		wayland/waylandBackend.hpp
		wayland/waylandCairo.hpp
		wayland/waylandUtils.hpp
		wayland/waylandWindowContext.hpp
		wayland/waylandInclude.hpp
	)
	if(NY_WithGL)
		set(H_WAYLAND ${H_WAYLAND}
			wayland/waylandEgl.hpp
			)		
	endif(NY_WithGL)

	install(FILES ${H_WAYLAND} DESTINATION include/ny/wayland)

endif(NY_WithWayland)



