#include 1. subdir cmake
if(WithX11)
	Message("x11 yoo")
endif(WithX11)

#window___________________________________________________________________________________
set(H_WINDOW 
	window/window.hpp
	window/frame.hpp
	window/widget.hpp
	window/widgets.hpp
	window/dialog.hpp
	window/style.hpp
	window/windowDefs.hpp
	window/windowEvents.hpp
	)

#graphics___________________________________________________________________________________
set(H_GRAPHICS 
	graphics/drawContext.hpp
	graphics/shape.hpp
	graphics/color.hpp
	graphics/cairo.hpp
	graphics/font.hpp
	graphics/freeType.hpp
	graphics/image.hpp
	graphics/texture.hpp
	graphics/transformable.hpp
	)

if(WithGL)
	set(H_GRAPHICS ${H_GRAPHICS}
	graphics/gl/glContext.hpp
	graphics/gl/glDC.hpp
	graphics/gl/shader.hpp
	)
endif(WithGL)

#backends___________________________________________________________________________________
set(H_BACKENDS 
	backends/backend.hpp
	backends/appContext.hpp
	backends/windowContext.hpp
	)

#x11___________________________________________________________________________________
if(WithX11)
	set(H_BACKENDS ${H_BACKENDS}
		backends/x11/appContext.hpp
		backends/x11/backend.hpp
		backends/x11/cairo.hpp
		backends/x11/defs.hpp
		backends/x11/utils.hpp
		backends/x11/windowContext.hpp
		)
	if(WithGL)
		set(H_BACKENDS ${H_BACKENDS}
			backends/x11/egl.hpp
			backends/x11/glx.hpp
			)		
	endif(WithGL)
endif(WithX11)


#wayland___________________________________________________________________________________
if(WithWayland)
	set(H_BACKENDS ${H_BACKENDS}
		backends/wayland/appContext.hpp
		backends/wayland/backend.hpp
		backends/wayland/cairo.hpp
		backends/wayland/defs.hpp
		backends/wayland/utils.hpp
		backends/wayland/windowContext.hpp
	)
	if(WithGL)
		set(H_BACKENDS ${H_BACKENDS}
			backends/wayland/gl.hpp
			)		
	endif(WithGL)
endif(WithWayland)

#app___________________________________________________________________________________
set(H_APP 
	app/app.hpp
	app/cursor.hpp
	app/dnd.hpp
	app/error.hpp
	app/event.hpp
	app/eventHandler.hpp
	app/file.hpp
	app/keyboard.hpp
	app/mouse.hpp
	app/surface.hpp
	)

set(H_UTILS 
	utils/vec.hpp
	utils/mat.hpp
	utils/rect.hpp
	utils/callback.hpp
	utils/thread.hpp
	utils/time.hpp
	utils/region.hpp
	utils/proto.hpp
	utils/misc.hpp
	utils/animation.hpp
	utils/refVec.hpp
	utils/nonCopyable.hpp
	)

set(H_NYFILES
	config.h
	ny.hpp
	include.hpp
	)

install(FILES ${H_NYFILES} DESTINATION include/ny)
install(FILES ${H_APP} DESTINATION include/ny/app)
install(FILES ${H_WINDOW} DESTINATION include/ny/window)
install(FILES ${H_UTILS} DESTINATION include/ny/utils)
install(FILES ${H_BACKENDS} DESTINATION include/ny/backends)
install(FILES ${H_GRAPHICS} DESTINATION include/ny/graphics)



