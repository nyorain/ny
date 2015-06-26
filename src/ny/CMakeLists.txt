#src 1. subdir cmake

#window___________________________________________________________________________________
set(S_WINDOW 
	window/window.cpp
	window/frame.cpp
	window/widget.cpp
	window/widgets.cpp
	window/dialog.cpp
	window/style.cpp
	window/headerbar.cpp
	)

#graphics___________________________________________________________________________________
set(S_GRAPHICS 
	graphics/drawContext.cpp
	graphics/shape.cpp
	graphics/color.cpp
	graphics/cairo.cpp
	graphics/font.cpp
	graphics/freeType.cpp
	graphics/image.cpp
	graphics/texture.cpp
	)

if(NY_WithGL)
	set(S_GRAPHICS ${S_GRAPHICS}
		graphics/gl/glContext.cpp
		graphics/gl/glDC.cpp
		graphics/gl/shader.cpp
		)
endif(NY_WithGL)

#backends___________________________________________________________________________________
set(S_BACKENDS 
	backends/backend.cpp
	backends/appContext.cpp
	backends/windowContext.cpp
	)

#x11___________________________________________________________________________________
if(NY_WithX11)
	set(S_BACKENDS ${S_BACKENDS}
		backends/x11/appContext.cpp
		backends/x11/backend.cpp
		backends/x11/cairo.cpp
		backends/x11/defs.cpp
		backends/x11/utils.cpp
		backends/x11/windowContext.cpp
		)
	if(NY_WithGL)
		set(S_BACKENDS ${S_BACKENDS}
			backends/x11/egl.cpp
			backends/x11/glx.cpp
			)		
	endif(NY_WithGL)
endif(NY_WithX11)


#wayland___________________________________________________________________________________
if(NY_WithWayland)
	set(S_BACKENDS ${S_BACKENDS}
		backends/wayland/appContext.cpp
		backends/wayland/backend.cpp
		backends/wayland/cairo.cpp
		backends/wayland/defs.cpp
		backends/wayland/utils.cpp
		backends/wayland/windowContext.cpp
		)
	if(NY_WithGL)
		set(S_BACKENDS ${S_BACKENDS}
			backends/wayland/gl.cpp
			)		
	endif(NY_WithGL)
endif(NY_WithWayland)

#app___________________________________________________________________________________
set(S_APP 
	app/app.cpp
	app/cursor.cpp
	app/data.cpp
	app/error.cpp
	app/event.cpp
	app/eventHandler.cpp
	app/file.cpp
	app/keyboard.cpp
	app/mouse.cpp
	app/touch.cpp
	)

set(FreeTypeCFlags "-I/usr/include/freetype2")
set(FreeTypeLFlags "-lfreetype")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${FreeTypeCFlags})
set(CMAKE_SHARED_FLAGS ${CMAKE_SHARED_FLAGS} ${FreeTypeLFlags})

add_library(ny SHARED ${S_WINDOW} ${S_GRAPHICS} ${S_BACKENDS} ${S_APP})
target_compile_features(ny PRIVATE cxx_range_for)
#cotire(ny)

target_link_libraries(ny wayland-cursor wayland-client wayland-egl cairo pthread glbinding X11)

install (TARGETS ny 
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)

