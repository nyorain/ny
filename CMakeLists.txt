cmake_minimum_required (VERSION 3.1.0)

project(ny)

#dump only this version on a new release
set(vmajor 0)
set(vminor 1)
set(vpatch 0)

option(BuildExamples "Build the ny examples" off)
option(BuildTests "Build the ny tests" off)
option(Debug "Include debug symbols" on)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17")

#include dirs
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include") #config.hpp
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

#external dirs
link_directories("${CMAKE_SOURCE_DIR}/external/tdm-gcc64")
include_directories("${CMAKE_SOURCE_DIR}/external/headers")

#cmake options
# set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH true)

if(Debug)
	set(CMAKE_BUILD_TYPE Debug)
else()
	set(CMAKE_BUILD_TYPE Release)
endif()

#to disable ninja byproduct warning
cmake_policy(SET CMP0058 NEW)

# TODO: export to extra module
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
		message("[config] Build option " ${name} " will be set to automatic")
	endif()
endfunction()

# check for auto options
function(checkConfigOption var pkg found apply)
	if(${var} EQUAL 2)
		find_package(${pkg} QUIET)

		if(found)
			message("[config] Found " ${pkg})
			if(apply)
				set(${var} 1 CACHE STRING "Build config option" FORCE)
			endif()
		else()
			message("[config] Failed to find " ${pkg})
			set(${var} 0 CACHE STRING "Build config option" FORCE)
		endif()
	endif()
endfunction()

# basic options defaulted to 2 (auto)
configOption(WithGL 2)
configOption(WithEGL 2)
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

# parsed the config option from
checkConfigOption(WithGL OpenGL OPENGL_FOUND 1)
checkConfigOption(WithEGL EGL EGL_FOUND 1)
checkConfigOption(WithVulkan Vulkan VULKAN_FOUND 1)

if(UNIX)
	checkConfigOption(WithX11 Xlib XLIB_FOUND 0)
	checkConfigOption(WithX11 XCB XCB_FOUND 1) #something about modules?
	checkConfigOption(WithWayland Wayland WAYLAND_FOUND 1)
endif()

#check for invalid configurations
#egl without gl
if(WithEGL AND NOT WithGL)
	message("[config] EGL will be disabled because it requires GL")
	set(WithEGL 0)
endif()

#no backend
if(NOT WithWayland AND NOT WithX11 AND NOT WithWinapi)
	message(FATAL_ERROR "[config] Cannot build ny without backend")
endif()

#configure nytl
#on unix system, one may have nytl already installed (or it will be pulled by a pkg manager)
#then it does not make sense to include its own copy.
#otherwise the nytl headers will be copied to the other external dependencies headers
if(NOT nytlLocation)
	find_package(nytl 0.2.0 QUIET)
	if(nytl_FOUND)
		message("[config] Found nytl at " ${nytl_INCLUDE_DIRS})
		set(nytlLocation "system: " ${nytl_INCLUDE_DIRS} CACHE STRING "Which nytl to use")
		set(nytlOwn off)
		include_directories(${nytl_INCLUDE_DIRS})
	else()
		message("[config] Couldnt find nytl. Will copy included headers")
		set(nytlLocation "own" CACHE STRING "Which nytl to use")
		set(nytlOwn on)
		add_subdirectory(external)
		include_directories("${CMAKE_CURRENT_BINARY_DIR}/external/install/include")
	endif()
endif()


#config file
#first set all macros (they have NY_ prefix since they are macros)
set(NY_WithGL ${WithGL})
set(NY_WithEGL ${WithEGL})
set(NY_WithWinapi ${WithWinapi})
set(NY_WithX11 ${WithX11})
set(NY_WithWayland ${WithWayland})

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/ny/config.hpp.in"
	"${CMAKE_CURRENT_BINARY_DIR}/include/ny/config.hpp")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/ny/config.hpp" DESTINATION include/ny)

#pkg-config
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/ny.pc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/ny.pc"
	@ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ny.pc" DESTINATION lib/pkgconfig)

#subdirs
add_subdirectory(src/ny)
add_subdirectory(include/ny)

if(BuildExamples)
	add_subdirectory(examples)
endif()

# uninstall target
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall 
	COMMAND ${CMAKE_COMMAND} -P 
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake)

#output configuration
message("\t\nFinal Configuration:\n")
message("\tWithWinapi:\t\t" ${WithWinapi})
message("\tWithWayland:\t\t" ${WithWayland})
message("\tWithX11:\t\t" ${WithX11})
message("\tWithGL:\t\t\t" ${WithGL})
message("\tWithEGL:\t\t" ${WithEGL})
message("\tWithVulkan:\t\t" ${WithVulkan})
message("\n")
message("\tnytl build:\t\t" ${nytlLocation})
message("\n")
message("\tny version:\t\t" ${vmajor} "." ${vminor} "." ${vpatch})
message("\n")
