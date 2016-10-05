set(include_backend ${default_include})
set(libs_backend ${default_libs} ny-base)
set(src_backend
	backend.cpp
	integration/surface.cpp)

if(WithWinapi)
	list(APPEND src_backend
		winapi/appContext.cpp
		winapi/backend.cpp
		winapi/util.cpp
		winapi/com.cpp
		winapi/input.cpp
		winapi/surface.cpp
		winapi/windowContext.cpp)

	list(APPEND libs_backend ${libs_backend} gdi32 gdiplus)

	if(WithGL)
		list(APPEND src_backend winapi/wgl.cpp winapi/wglApi.c)
	endif()

	if(WithVulkan)
		list(APPEND src_backend winapi/vulkan.cpp)
	endif()

	if(WithCairo)
		list(APPEND src_backend winapi/cairo.cpp)
	endif()
endif()

if(WithX11)
	list(APPEND src_backend
		x11/appContext.cpp
		x11/backend.cpp
		x11/util.cpp
		x11/surface.cpp
		x11/input.cpp
		x11/windowContext.cpp)

	list(APPEND libs_backend
		${XCB_LIBRARIES}
		${X11_LIBRARIES}
		${X11_Xcursor_LIB})
	list(APPEND include_backend
		${XCB_INCLUDE_DIRS}
		${X11_XCB_INCLUDE_DIR}
		${X11_Xcursor_INCLUDE_PATH})

	list(APPEND libs_backend X11 xcb xcb-icccm xcb-ewmh X11-xcb pthread)

	if(WithGL)
		# list(APPEND src_backend x11/glx.cpp x11/glxApi.cpp)
	endif()

	if(WithCairo)
		list(APPEND src_backend x11/cairo.cpp)
	endif()

	if(WithVulkan)
		list(APPEND src_backend x11/vulkan.cpp)
	endif()
endif()

if(WithWayland)
	list(APPEND src_backend
		wayland/appContext.cpp
		wayland/backend.cpp
		wayland/input.cpp
		wayland/util.cpp
		wayland/surface.cpp
		wayland/windowContext.cpp
		wayland/interfaces.cpp
		wayland/xdg-shell-protocol.c)

	list(APPEND libs_backend ${WAYLAND_LIBRARIES})
	list(APPEND include_backend ${WAYLAND_INCLUDE_DIRS})

	if(WithEGL)
		list(APPEND src_backend wayland/egl.cpp)
	endif()

	if(WithCairo)
		list(APPEND src_backend wayland/cairo.cpp)
	endif()

	if(WithVulkan)
		list(APPEND src_backend wayland/vulkan.cpp)
	endif()
endif()

if(WithX11 OR WithWayland)
	list(APPEND src_backend common/xkb.cpp)
	list(APPEND libs_backend ${XKBCOMMON_LIBRARIES})
	list(APPEND include_backend ${XKBCOMMON_INCLUDE_DIRS})
endif()

if(WithGL)
	list(APPEND src_backend common/gl.cpp)
	list(APPEND libs_backend ${OPENGL_LIBRARY})
	list(APPEND include_backend ${OPENGL_INCLUDE_DIR})
endif()

if(WithEGL)
	list(APPEND src_backend common/egl.cpp)
	list(APPEND libs_backend ${EGL_LIBRARIES})
	list(APPEND include_backend ${EGL_INCLUDE_DIRS})
endif()

if(WithVulkan)
	list(APPEND libs_backend ${Vulkan_LIBRARY})
	list(APPEND include_backend ${Vulkan_INCLUDE_DIR})
endif()

if(WithCairo)
	list(APPEND src_backend integration/cairo.cpp)
	list(APPEND libs_backend ${CAIRO_LIBRARIES})
	list(APPEND include_backend ${CAIRO_INCLUDE_DIRS})
endif()

add_library(ny-backend SHARED ${src_backend})
target_link_libraries(ny-backend ${libs_backend})
target_include_directories(ny-backend PUBLIC ${include_backend})

if(nytl_own)
	add_dependencies(ny-backend nytl_ep)
endif()

#install
install (TARGETS ny-backend
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
