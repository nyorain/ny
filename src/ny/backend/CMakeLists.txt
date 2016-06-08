set(include-backend ${include-ny})
set(libs-backend ${libs-backend} ny-base ny-draw)

set(src-backend backend.cpp)

if(${WithWinapi})
	set(src-backend ${src-backend}
		winapi/appContext.cpp
		winapi/backend.cpp
		winapi/util.cpp
		winapi/gdi.cpp
		winapi/wgl.cpp
		winapi/windowContext.cpp)

	set(libs-backend ${libs-backend} gdi32 gdiplus)

	if(${WithGL})
		set(src-backend ${src-backend} winapi/wgl.cpp winapi/wgl/glad_wgl.c)
		set(libs-backend ${libs-backend} OpenGL32 gdi32)
	endif(${WithGL})

endif()

if(${WithX11})
	set(src-backend ${src-backend}
		x11/appContext.cpp
		x11/backend.cpp
		x11/util.cpp
		x11/windowContext.cpp)

	set(libs-backend ${libs-backend} X11 xcb xcb-icccm xcb-ewmh X11-xcb pthread)

	if(${WithGL})
		set(src-backend ${src-backend} x11/glx.cpp)
		set(libs-backend ${libs-backend} GL)
	endif()

	if(${WithCairo})
		set(src-backend ${src-backend} x11/cairo.cpp)
		set(libs-backend ${libs-backend} cairo)
	endif()

endif()

if(${WithWayland})
	set(src-backend ${src-backend}
		wayland/appContext.cpp
		wayland/backend.cpp
		wayland/util.cpp
		wayland/windowContext.cpp
		wayland/interfaces.cpp
		wayland/xdg-shell-client-protocol.c)

	if(${WithGL} and ${WithEGL})
		set(src-backend ${src-backend} wayland/egl.cpp)
	endif()
endif()

add_library(ny-backend SHARED ${src-backend})
target_link_libraries(ny-backend ${libs-backend})
target_include_directories(ny-backend PUBLIC ${include-backend})

set(targets-backend ny-backend)

#install
install (TARGETS ${targets-backend}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
