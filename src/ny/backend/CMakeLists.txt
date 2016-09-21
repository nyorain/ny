set(include_backend ${default_include})
set(libs_backend ${default_libs} evg ny-base)
set(src_backend backend.cpp)

if(WithWinapi)
	list(APPEND src_backend
		winapi/appContext.cpp
		winapi/backend.cpp
		winapi/util.cpp
		# winapi/gdi.cpp
		winapi/com.cpp
		winapi/input.cpp
		winapi/windowContext.cpp)

	list(APPEND libs_backend ${libs_backend} gdi32 gdiplus)

	if(WithGL)
		# list(APPEND src_backend winapi/wgl.cpp winapi/wglApi.cpp)
		# list(APPEND libs_backend OpenGL32)
	endif()
endif()

if(WithX11)
	list(APPEND src_backend
		x11/appContext.cpp
		x11/backend.cpp
		x11/util.cpp
		x11/input.cpp
		x11/cairo.cpp
		x11/windowContext.cpp)

	list(APPEND libs_backend ${XCB_LIBRARIES} ${X11_LIBRARIES})
	list(APPEND include_backend ${XCB_INCLUDE_DIRS} ${X11_XCB_INCLUDE_DIR})

	list(APPEND libs_backend X11 xcb xcb-icccm xcb-ewmh X11-xcb pthread)

	if(WithGL)
		# list(APPEND src_backend x11/glx.cpp x11/glxApi.cpp)
		# list(APPEND libs_backend GL)
	endif()
endif()

if(WithWayland)
	list(APPEND src_backend
		wayland/appContext.cpp
		wayland/backend.cpp
		wayland/input.cpp
		wayland/cairo.cpp
		wayland/util.cpp
		wayland/windowContext.cpp
		wayland/interfaces.cpp
		wayland/xdg-shell-protocol.c)

	list(APPEND libs_backend ${WAYLAND_LIBRARIES})
	list(APPEND include_backend ${WAYLAND_INCLUDE_DIRS})

	if(WithEGL)
		# list(APPEND src_backend wayland/egl.cpp)
	endif()
endif()

if(WithX11 OR WithWayland)
	list(APPEND src_backend common/xkb.cpp)
	list(APPEND libs_backend ${XKBCOMMON_LIBRARIES})
	list(APPEND include_backend ${XKBCOMMON_INCLUDE_DIRS})
endif()

if(WithGL)
	list(APPEND src_backend common/gl.cpp)
endif()

if(WithEGL)
	list(APPEND src_backend common/egl.cpp)
endif()

if(WithVulkan)
	list(APPEND src_backend common/vulkan.cpp)
	list(APPEND libs_backend vpp ${vulkan_LIBS})
endif()

add_library(ny-backend SHARED ${src_backend})
target_link_libraries(ny-backend ${libs_backend})
target_include_directories(ny-backend PUBLIC ${include_backend})

if(nytl_own)
	add_dependencies(ny-app nytl_ep)
endif()

if(evg_own)
	add_dependencies(ny-app evg_ep)
endif()

#install
install (TARGETS ny-backend
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
