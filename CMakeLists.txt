#toplevel cmake

cmake_minimum_required (VERSION 2.8)

project(ny)

set(NY_VMAJOR 0)
set(NY_VMINOR 1)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

option(NY_BUILD_EXAMPLES "build the ny examples" ON)

option(NY_WithGL "Include GL functions and backends" ON)
option(NY_WithGLBinding "Use GLBinding Instead of static GL" ON)
option(NY_WithCairo "Include Cairo fuctions and drawContext" ON)
option(NY_WithFreeType "Enable FreeType for font handling" ON)

if(UNIX)
	option(NY_WithX11 "Include X11 functions and backends" ON)
	option(NY_WithWayland "Include Wayland functions and backends" ON)
endif(UNIX)

if(Win32)
	option(NY_WithWinapi "Include Winapi functions and backends" ON)
endif(Win32)

#pch
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
include(cotire)


configure_file	( "${CMAKE_CURRENT_SOURCE_DIR}/src/ny/config.h.in"
		  "${CMAKE_CURRENT_SOURCE_DIR}/include/ny/config.h"
	     	)

add_subdirectory(src/ny)
add_subdirectory(include/ny)

if(NY_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif(NY_BUILD_EXAMPLES)

# uninstall target
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/uninstall.cmake.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/moudles/uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/uninstall.cmake)

SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) 
