cmake_minimum_required (VERSION 3.1.0)

project(ny)

#increase this version on a new release
set(vmajor 0)
set(vminor 1)
set(vpatch 0)

#default options
option(BuildExamples "Build the ny examples" off)
option(BuildTests "Build the ny tests" off)
option(Debug "Include debug symbols" on)

#to disable ninja byproduct warning
cmake_policy(SET CMP0058 NEW)

#include dirs
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include") #config.hpp

#external dirs
include_directories("${CMAKE_BINARY_DIR}/external/install/include")
link_directories("${CMAKE_BINARY_DIR}/external/install/lib")

#cmake options
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17") # C++ 17
# set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
# set(CMAKE_BUILD_WITH_INSTALL_RPATH true)

#build type
if(Debug)
	set(CMAKE_BUILD_TYPE Debug)
else()
	set(CMAKE_BUILD_TYPE Release)
endif()


#config options
#check for available packages
find_package(OpenGL)
find_package(EGL)
find_package(Vulkan)
find_package(Cairo)

if(UNIX)
	find_package(X11 COMPONENTS Xcursor)
	find_package(XCB COMPONENTS ewmh xkb image icccm)
	find_package(Wayland)
	find_package(XKBCommon)
endif()

#dependent options
include(CMakeDependentOption)

cmake_dependent_option(WithGL "Build ny with gl support" on
	"OPENGL_FOUND" off)
cmake_dependent_option(WithEGL "Built ny with egl support" on
	"OPENGL_FOUND;EGL_FOUND" off)
cmake_dependent_option(WithVulkan "Build ny with vulkan support" on
	"VULKAN_FOUND" off)
cmake_dependent_option(WithCairo "Build ny with cairo support" on
	"CAIRO_FOUND" off)
cmake_dependent_option(WithX11 "Build ny with a x11 backend" on
	"XKBCOMMON_FOUND;X11_FOUND;XCB_FOUND" off)
cmake_dependent_option(WithWayland "Build ny with a wayland backend" on
	"XKBCOMMON_FOUND;WAYLAND_FOUND" off)
cmake_dependent_option(WithWinapi "Build ny with a winapi backend" on
	"WIN32" off)

#check for invalid configurations
#no backend
if(NOT WithWayland AND NOT WithX11 AND NOT WithWinapi)
	message(FATAL_ERROR "Cannot build ny without backend")
endif()

#no drawing api
if(NOT WithCairo AND NOT WithEGL AND NOT WithVulkan AND NOT WithWinapi)
	message(WARNING "Building ny without drawing backends")
endif()

#external projects
#configure evg
set(vulkan-condition false)
if(WithVulkan AND NOT vpp_location)
	set(vulkan-condition true)
endif()

if(NOT evg_location AND NOT nytl_location AND vulkan-condition)
	set(evg-components true)
	find_package(evg 0.1.0 QUIET OPTIONAL_COMPONENTS cairo gl vulkan gdi)
	find_package(nytl 0.2.0 QUIET)

	if(WithVulkan)
		find_package(vpp 0.2.0 QUIET)
		if(NOT VPP_FOUND)
			set(evg-components 0)
		endif()
		if(WithVulkan AND NOT EVG_VULKAN_FOUND)
			set(evg-components false)
		endif()
	endif()

	if(WithGL AND NOT EVG_GL_FOUND)
		set(evg-components false)
	endif()

	if(WithCairo AND NOT EVG_CAIRO_FOUND)
		set(evg-components false)
	endif()

	if(WithWinapi AND NOT EVG_GDI_FOUND)
		set(evg-components flase)
	endif()

	if(EVG_FOUND AND NYTL_FOUND AND evg-components)
		message("[config] Found evg headers at " ${EVG_INCLUDE_DIRS})
		message("[config] Found nytl headers at " ${NYTL_INCLUDE_DIRS})
		set(evg_location ${EVG_INCLUDE_DIRS} CACHE STRING "Which evg to use")
		set(nytl_location ${NYTL_INCLUDE_DIRS} CACHE STRING "Which nytl to use")
		set(vpp_location ${VPP_INCLUDE_DIRS} CACHE STRING "Which vpp to use")
		set(evg_build off CACHE INTERNAL "")
	else()
		message("[config] Could not find valid evg and nytl. Will build own version")
		set(evg_location "own" CACHE STRING "Which evg to use")
		set(nytl_location "own (evg)" CACHE STRING "Which nytl to use")
		set(vpp_location "own (evg)" CACHE STRING "Which vpp to use")
		set(evg_build on CACHE INTERNAL "")
	endif()
endif()

#config file
#first set all macros (they have NY_ prefix since they are macros)
set(NY_WithGL ${WithGL})
set(NY_WithEGL ${WithEGL})
set(NY_WithVulkan ${WithVulkan})
set(NY_WithCairo ${WithCairo})
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
add_subdirectory(include)
add_subdirectory(external)

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
	${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake)

#output configuration
#function that normalizes bool vars to 0 or 1.
function(normalize arg)
	if(${arg})
		set(${arg} 1 PARENT_SCOPE)
	else()
		set(${arg} 0 PARENT_SCOPE)
	endif()
endfunction()

#normalize all vars
normalize(WithGL)
normalize(WithEGL)
normalize(WithVulkan)
normalize(WithWinapi)
normalize(WithX11)
normalize(WithWayland)

#output
message("\t\nFinal Configuration:\n")

message("\tWithWinapi:\t\t" ${WithWinapi})
message("\tWithWayland:\t\t" ${WithWayland})
message("\tWithX11:\t\t" ${WithX11})
message("\tWithGL:\t\t\t" ${WithGL})
message("\tWithEGL:\t\t" ${WithEGL})
message("\tWithVulkan:\t\t" ${WithVulkan})
message("\n")

message("\tevg build:\t\t" ${evg_location})
message("\tnytl build:\t\t" ${nytl_location})

if(WithVulkan)
	message("\tvpp build:\t\t" ${vpp_location})
endif()

message("\n\tny version:\t\t" ${vmajor} "." ${vminor} "." ${vpatch})
message("\n")

# contributions to the ny cmake files are highly appreciated. lol
