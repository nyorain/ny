cmake_minimum_required (VERSION 3.1.0)

project(ny)

set(vmajor 0)
set(vminor 1)
set(vpatch 0)

option(BuildExamples "build the ny examples" ON)
option(BuildTests "build the ny tests" OFF)
option(BuildDebug "include debug symbols" ON)

#include dirs
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/src")

#external dirs
link_directories("${CMAKE_SOURCE_DIR}/external/tdm-gcc64")
include_directories("${CMAKE_SOURCE_DIR}/external/headers")

#cmake options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# sets a config option to auto if not set by command line
# these config vars are not options but integers, since there default value
# (if they are not yet determined or specified by command line) is 2 which
# means auto. Then they are set to ON if possible and to OFF if not possible.
# E.g. by default WithCairo=2, then cmake will check if cairo can be found.
# If it can, WithCairo will be set to 1 and ny will be built with cairo.
# Otherwise it will be set to 0 and ny will be built without it.
function(configOption name default)
	if(NOT DEFINED ${name})
		set(${name} ${default} CACHE STRING "Build config option")
	endif()
endfunction()

# check for auto options
function(checkConfigOption var pkg found apply)
	if(${${var}} EQUAL 2)
		message("--- Build option " ${var} " set to auto, trying to find dependencies.")
		find_package(${pkg} QUIET)

		if(${${found}} AND ${apply})
			message("--- ---> Found " ${pkg})
			set(${var} 1 CACHE STRING "Build config option" FORCE)
		else()
			message("--- ---> Failed to find " ${pkg})
			set(${var} 0 CACHE STRING "Build config option" FORCE)
		endif()
	endif()
endfunction()

# basic options defaulted to 2 (auto)
configOption(WithFreeType 2)
configOption(WithGL 2)
configOption(WithEGL 2)
configOption(WithCairo 2)
configOption(WithVulkan 2)

if(UNIX)
	configOption(WithX11 2)
	configOption(WithWayland 2)
	configOption(WithWinapi 0)
elseif(WIN32)
	configOption(WithWinapi 1)
	configOption(WithX11 0)
	configOption(WithWayland 0)
endif()

checkConfigOption(WithFreeType FreeType FREETYPE_FOUND 1)
checkConfigOption(WithGL OpenGL OPENGL_FOUND 1)
checkConfigOption(WithEGL EGL EGL_FOUND 1)
checkConfigOption(WithCairo Cairo CAIRO_FOUND 1)
checkConfigOption(WithVulkan Vulkan VULKAN_FOUND 1)

if(UNIX)
	# TODO
	checkConfigOption(WithX11 Xlib XLIB_FOUND 0)
	checkConfigOption(WithX11 XCB XCB_FOUND 1) #something about modules?
	checkConfigOption(WithWayland Wayland WAYLAND_FOUND 1)
endif()

if(BuildDebug)
	set(CMAKE_BUILD_TYPE Debug)
else()
	set(CMAKE_BUILD_TYPE Release)
endif()

#config, TODO: install
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/ny/config.hpp.in"
	"${CMAKE_CURRENT_BINARY_DIR}/include/ny/config.hpp")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/ny/config.hpp" DESTINATION include/ny)

#configure FreeType - TODO
#copy binaries shipped (link to them only on windows)

#configure nytl
find_package(nytl "0.1" QUIET)
if(${nytl_FOUND} AND 0)
	set(${nytl_build} "system: " ${nytl_INCLUDE_DIRS})
	include_directories(${nytl_INCLUDE_DIRS})
else()
	set(${nytl_build} "own")
	set(own_nytl_build on)
	add_subdirectory(external)
	include_directories("${CMAKE_CURRENT_BINARY_DIR}/external/install/include")
	link_directories("${CMAKE_CURRENT_BINARY_DIR}/external/install/lib")
	link_directories("${CMAKE_CURRENT_BINARY_DIR}/external/install/bin")
endif()

add_subdirectory(src/ny)
add_subdirectory(include/ny)

if(BuildExamples)
	add_subdirectory(examples)
endif()

#to disable ninja byproduct warning
cmake_policy(SET CMP0058 NEW)

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

#output configuration
message("\t\nFinal Configuration:\n")
message("\tWithWinapi:\t\t" ${WithWinapi})
message("\tWithWayland:\t\t" ${WithWayland})
message("\tWithX11:\t\t" ${WithX11})
message("\tWithCairo:\t\t" ${WithCairo})
message("\tWithGL:\t\t\t" ${WithGL})
message("\tWithEGL:\t\t" ${WithEGL})
message("\tWithVulkan:\t\t" ${WithVulkan})
message("\tWithFreeType:\t\t" ${WithFreeType})
message("\n")
message("\tUsed nytl build:\t\t" ${nytl_build})
message("\n")
