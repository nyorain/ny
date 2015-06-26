#toplevel cmake

cmake_minimum_required (VERSION 2.6)

project(ny)

set(NY_VMAJOR 0)
set(NY_VMINOR 1)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

option(WithGL "Include GL functions and backends" ON)
option(WithGLBinding "Use GLBinding Instead of static GL" ON)
option(WithCairo "Include Cairo fuctions and drawContext" ON)
option(WithFreeType "Enable FreeType for font handling" ON)

if(UNIX)
	option(WithX11 "Include X11 functions and backends" ON)
	option(WithWayland "Include Wayland functions and backends" ON)
endif(UNIX)

if(Win32)
	option(WithWinapi "Include Winapi functions and backends" ON)
endif(Win32)



configure_file	( "${CMAKE_CURRENT_SOURCE_DIR}/src/ny/config.h.in"
		  "${CMAKE_CURRENT_SOURCE_DIR}/include/ny/config.h"
	     	)

add_subdirectory(src/ny)
add_subdirectory(include/ny)
add_subdirectory(examples)

# uninstall target
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake)

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) 

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)





