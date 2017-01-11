# Copyright (c) 2017 nyorain
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

set(ny_include)
set(ny_defs)
set(ny_libs)

# all common src files
set(ny_src
	log.cpp
	config.cpp
	cursor.cpp
	image.cpp
	dataExchange.cpp
	key.cpp
	mouseButton.cpp
	backend.cpp
	library.cpp)

# =======================================================================================
# Winapi backend
if(WithWinapi)
	list(APPEND ny_src
		winapi/appContext.cpp
		winapi/backend.cpp
		winapi/bufferSurface.cpp
		winapi/com.cpp
		winapi/input.cpp
		winapi/util.cpp
		winapi/windowContext.cpp)

	list(APPEND ny_libs gdi32 gdiplus Dwmapi Shlwapi)

	if(WithGl)
		list(APPEND ny_src winapi/wgl.cpp winapi/wglApi.c)
	endif()

	if(WithVulkan)
		list(APPEND ny_src winapi/vulkan.cpp)
	endif()
endif()

# =======================================================================================
# X11 backend
if(WithX11)
	list(APPEND ny_src
		x11/appContext.cpp
		x11/backend.cpp
		x11/bufferSurface.cpp
		x11/dataExchange.cpp
		x11/input.cpp
		x11/util.cpp
		x11/windowContext.cpp)

	list(APPEND ny_libs
		${XCB_LIBRARIES}
		${X11_LIBRARIES}
		${X11_Xcursor_LIB})
	list(APPEND ny_include
		${XCB_INCLUDE_DIRS}
		${X11_XCB_INCLUDE_DIR}
		${X11_Xcursor_INCLUDE_PATH})

	list(APPEND ny_libs X11 xcb xcb-icccm xcb-ewmh X11-xcb pthread)

	if(WithGl)
		list(APPEND ny_src x11/glx.cpp x11/glxApi.c)
	endif()

	if(WithVulkan)
		list(APPEND ny_src x11/vulkan.cpp)
	endif()
endif()

# =======================================================================================
# Wayland backend
if(WithWayland)
	list(APPEND ny_src
		wayland/appContext.cpp
		wayland/backend.cpp
		wayland/bufferSurface.cpp
		wayland/dataExchange.cpp
		wayland/input.cpp
		wayland/util.cpp
		wayland/windowContext.cpp

		wayland/protocols/xdg-shell-v5.c
		wayland/protocols/xdg-shell-v6.c)

	list(APPEND ny_libs ${WAYLAND_LIBRARIES})
	list(APPEND ny_include ${WAYLAND_INCLUDE_DIRS})

	if(WithEgl)
		list(APPEND ny_src wayland/egl.cpp)
	endif()

	if(WithVulkan)
		list(APPEND ny_src wayland/vulkan.cpp)
	endif()
endif()

# xkbcommon, unix
if(WithX11 OR WithWayland)
	list(APPEND ny_src common/xkb.cpp common/unix.cpp)
	list(APPEND ny_libs ${XKBCOMMON_LIBRARIES})
	list(APPEND ny_include ${XKBCOMMON_INCLUDE_DIRS})
endif()

# gl
if(WithGl)
	list(APPEND ny_src common/gl.cpp)
	list(APPEND ny_libs ${OPENGL_LIBRARY})
	list(APPEND ny_include ${OPENGL_INCLUDE_DIR})
endif()

# egl
if(WithEgl)
	list(APPEND ny_src common/egl.cpp)
	list(APPEND ny_libs ${EGL_LIBRARIES})
	list(APPEND ny_include ${EGL_INCLUDE_DIRS})
endif()

# vulkan
if(WithVulkan)
	list(APPEND ny_libs ${Vulkan_LIBRARY})
	list(APPEND ny_include ${Vulkan_INCLUDE_DIR})
endif()

add_library(ny SHARED ${ny_src})
target_link_libraries(ny ${ny_libs})
target_include_directories(ny PUBLIC ${ny_include})
target_compile_definitions(ny PRIVATE ${ny_defs})

# nytl build dep
if(nytl_own AND DependOnNytl)
	add_dependencies(ny nytl_ep)
endif()

#install
install (TARGETS ny
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
