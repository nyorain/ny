cmake_minimum_required (VERSION 3.1.0)

project(ny)

#increase this version on a new release
set(vmajor 0)
set(vminor 1)
set(vpatch 0)

#default options
option(BuildExamples "Build the ny examples" off)
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
list(APPEND CMAKE_CXX_FLAGS "-std=c++17") # C++ 17
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/src/cmake")
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
	find_package(XCB COMPONENTS ewmh xkb image icccm shm)
	find_package(Wayland)
	find_package(XKBCommon)
endif()

# dependent options
include(CMakeDependentOption)

# integration/context support
cmake_dependent_option(WithGL "Build ny with gl support" on
	"OPENGL_FOUND" off)
cmake_dependent_option(WithEGL "Built ny with egl support" on
	"WithGL;EGL_FOUND" off)
cmake_dependent_option(WithVulkan "Build ny with vulkan support" on
	"VULKAN_FOUND" off)
cmake_dependent_option(WithCairo "Build with cairo integration" on
	"CAIRO_FOUND" off)

# backends
cmake_dependent_option(WithX11 "Build ny with a x11 backend" on
	"XKBCOMMON_FOUND;X11_FOUND;XCB_FOUND" off)
cmake_dependent_option(WithWayland "Build ny with a wayland backend" on
	"XKBCOMMON_FOUND;WAYLAND_FOUND" off)
cmake_dependent_option(WithWinapi "Build ny with a winapi backend" on
	"WIN32" off)

# check for invalid configurations
# no backend
if(NOT WithWayland AND NOT WithX11 AND NOT WithWinapi)
	message(FATAL_ERROR "Could not find any backend")
endif()

#test for nytl
find_package(nytl 0.2.0)
if(nytl_FOUND AND nytl_INCLUDE_DIRS EQUAL CMAKE_INSTALL_PREFIX)
	set(nytl_location ${nytl_INCLUDE_DIRS})
	set(nytl_own false)
else()
	set(nytl_location "own")
	set(nytl_own true)

	include(ExternalProject)
	set(ExternalProjectCMakeArgs
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/external/install
		-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

	ExternalProject_Add(nytl_ep
		PREFIX ${CMAKE_CURRENT_BINARY_DIR}/nytl
	  	GIT_REPOSITORY https://github.com/nyorain/nytl.git
		INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/external/install
		CMAKE_ARGS ${ExternalProjectCMakeArgs})

	ExternalProject_Add_Step(nytl_ep
		forceinstall
		DEPENDEES configure
		DEPENDERS install
		ALWAYS 1)
	install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/external/install/." DESTINATION .)
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

if(BuildExamples)
	add_subdirectory(src/examples)
endif()

# uninstall target
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/src/cmake/uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/src/cmake/uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P
	${CMAKE_CURRENT_BINARY_DIR}/src/cmake/uninstall.cmake)

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
normalize(WithCairo)

#output
message("\t\nFinal Configuration:\n")

message("\tWithWinapi:\t\t" ${WithWinapi})
message("\tWithWayland:\t\t" ${WithWayland})
message("\tWithX11:\t\t" ${WithX11})
message("\tWithGL:\t\t\t" ${WithGL})
message("\tWithEGL:\t\t" ${WithEGL})
message("\tWithVulkan:\t\t" ${WithVulkan})
message("\tWithCairo:\t\t" ${WithCairo})
message("\n")

message("\tnytl build:\t\t" ${nytl_location})

message("\n\tny version:\t\t" ${vmajor} "." ${vminor} "." ${vpatch})
message("\n")

# contributions to the ny cmake files are highly appreciated. lol
