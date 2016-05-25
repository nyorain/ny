set(include-backend ${include-ny})
set(libs-backend ${libs-backend} ny-base ny-draw)

set(src-backend
		backend.cpp)

if(NY_WithWinapi)
	set(src-backend ${src-backend}
		winapi/appContext.cpp
		winapi/backend.cpp
		winapi/util.cpp
		winapi/gdi.cpp
		winapi/wgl.cpp
		winapi/windowContext.cpp)

	set(libs-backend ${libs-backend} gdi32 gdiplus)

	if(NY_WithGL)
		set(src-backend ${src-backend} winapi/wgl.cpp winapi/wgl/glad_wgl.c)
		set(libs-backend ${libs-backend} OpenGL32)
	endif(NY_WithGL)

endif(NY_WithWinapi)

if(NY_WithX11)
	set(src-backend ${src-backend}
		x11/appContext.cpp
		x11/backend.cpp
		x11/util.cpp
		x11/windowContext.cpp)

	set(libs-backend ${libs-backend} X11 xcb xcb-icccm xcb-ewmh X11-xcb pthread)

	if(NY_WithGL)
		set(src-backend ${src-backend} x11/glx.cpp)
		set(libs-backend ${libs-backend} GL)
	endif(NY_WithGL)

	if(NY_WithCairo)
		set(src-backend ${src-backend} x11/cairo.cpp)
		set(libs-backend ${libs-backend} cairo)
	endif(NY_WithCairo)

endif(NY_WithX11)

if(NY_WithWayland)
	set(src-backend ${src-backend}
		wayland/appContext.cpp
		wayland/backend.cpp
		wayland/util.cpp
		wayland/windowContext.cpp
		wayland/interfaces.cpp
		wayland/xdg-shell-client-protocol.c)

	if(NY_WithEGL)
		set(src-backend ${src-backend} wayland/egl.cpp)
	endif(NY_WithGL)
endif(NY_WithWayland)

add_library(ny-backend SHARED ${src-backend})
target_link_libraries(ny-backend ${libs-backend})
target_include_directories(ny-backend PUBLIC ${include-backend})

set(targets-backend ny-backend)

#install
install (TARGETS ${targets-backend}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
