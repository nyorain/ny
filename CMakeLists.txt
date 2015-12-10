#toplevel cmake

cmake_minimum_required (VERSION 2.8)

project(ny)

set(NY_VMAJOR 0)
set(NY_VMINOR 1)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

option(NY_BUILD_EXAMPLES "build the ny examples" OFF)
option(NY_BUILD_TESTS "build the ny tests" OFF)

#todo
set(NY_WithGL 1)
set(NY_WithGLES 1)
set(NY_WithEGL 1)
set(NY_WithGlbinding 1)
set(NY_WithGlesbinding 1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

#config, TODO: install
configure_file	( "${CMAKE_CURRENT_SOURCE_DIR}/src/ny/config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/include/ny/config.h"
	     	)


add_subdirectory(src/ny)
add_subdirectory(include/ny)

if(NY_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif(NY_BUILD_EXAMPLES)

#pkg-config
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/ny.pc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/ny.pc"
	@ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ny.pc" DESTINATION lib/pkgconfig)

# uninstall target
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake)

SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
