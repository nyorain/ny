#src 1. subdir cmake

#window___________________________________________________________________________________
#include 1. subdir cmake

#general
set(SOURCE
	window.cpp
	frame.cpp
	widget.cpp
	widgets.cpp
	dialog.cpp
	style.cpp
	drawContext.cpp
	shape.cpp
	color.cpp
	cairo.cpp
	font.cpp
	freeType.cpp
	image.cpp
	texture.cpp
	app.cpp
	cursor.cpp
	data.cpp
	error.cpp
	event.cpp
	eventHandler.cpp
	file.cpp
	keyboard.cpp
	mouse.cpp
	surface.cpp
	backend.cpp
	appContext.cpp
	windowContext.cpp
	)

#specific backends
#gl___________________________________________________________________________________
if(NY_WithGL)
	set(SOURCE ${SOURCE}
	gl/glDrawContext.cpp
	gl/shader.cpp
	gl/egl.cpp
	)
endif(NY_WithGL)


#x11___________________________________________________________________________________
if(NY_WithX11)
	set(SOURCE ${SOURCE}
		x11/x11AppContext.cpp
		x11/x11Backend.cpp
		x11/x11Cairo.cpp
		x11/x11Util.cpp
		x11/x11WindowContext.cpp
		)

	if(NY_WithGL)
		set(SOURCE ${SOURCE}
			x11/x11Egl.cpp
			x11/glx.cpp
			)		
	endif(NY_WithGL)
endif(NY_WithX11)


#wayland___________________________________________________________________________________
if(NY_WithWayland)
	set(SOURCE ${SOURCE}
		wayland/waylandAppContext.cpp
		wayland/waylandBackend.cpp
		wayland/waylandCairo.cpp
		wayland/waylandUtil.cpp
		wayland/waylandWindowContext.cpp
		wayland/waylandInterfaces.cpp
		wayland/xdg-shell-protocol.c
	)
	if(NY_WithGL)
		set(SOURCE ${SOURCE}
			wayland/waylandEgl.cpp
			)		
	endif(NY_WithGL)
endif(NY_WithWayland)

set(FreeTypeCFlags "-I/usr/include/freetype2")
set(FreeTypeLFlags "-lfreetype")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${FreeTypeCFlags})
set(CMAKE_SHARED_FLAGS ${CMAKE_SHARED_FLAGS} ${FreeTypeLFlags})

add_library(ny SHARED ${SOURCE})
target_compile_features(ny PRIVATE cxx_range_for)

target_link_libraries(ny wayland-cursor wayland-client wayland-egl cairo pthread glbinding X11)

install (TARGETS ny 
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)

