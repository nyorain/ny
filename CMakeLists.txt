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
include_directories("${CMAKE_SOURCE_DIR}/external/install/include")
link_directories("${CMAKE_SOURCE_DIR}/external/install/lib")

#cmake options
# set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17") # C++ 17
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_BUILD_WITH_INSTALL_RPATH true)

#build type
if(Debug)
	set(CMAKE_BUILD_TYPE Debug)
else()
	set(CMAKE_BUILD_TYPE Release)
endif()


#config options
#check for available packages
find_package(OpenGL )
find_package(EGL )
find_package(Vulkan )

if(UNIX)
	find_package(X11 )
	find_package(XCB COMPONENTS ewmh xkb image icccm)
	find_package(Wayland )
	find_package(XKBCommon )
endif()

#dependent options
include(CMakeDependentOption)

cmake_dependent_option(WithGL "Build ny with gl support" on 
	"OPENGL_FOUND" off)
cmake_dependent_option(WithEGL "Built ny with egl support" on
	"OPENGL_FOUND;EGL_FOUND" off)
cmake_dependent_option(WithVulkan "Build ny with vulkan support" on 
	"VULKAN_FOUND" off)
cmake_dependent_option(WithX11 "Build ny with a x11 backend" on 
	"XKBCommon_FOUND;X11_FOUND;XCB_FOUND" off)
cmake_dependent_option(WithWayland "Build ny with a wayland backend" on 
	"XKBCommon_FOUND;Wayland_FOUND" off)
cmake_dependent_option(WithWinapi "Build ny with a winapi backend" on 
	"WIN32" off)

#check for invalid configurations
#no backend
if(NOT WithWayland AND NOT WithX11 AND NOT WithWinapi)
	message(FATAL_ERROR "[config] Cannot build ny without backend")
endif()

#external projects
#configure evg
if(NOT evg_location AND NOT nytl_location)
	find_package(evg 0.1.0 QUIET)
	find_package(nytl 0.2.0 QUIET)

	if(evg_FOUND AND nytl_FOUND)
		message("[config] Found evg headers at " ${evg_INCLUDE_DIRS})
		message("[config] Found nytl headers at " ${nytl_INCLUDE_DIRS})
		set(evg_location ${evg_INCLUDE_DIRS} CACHE STRING "Which evg to use")
		set(nytl_location ${nytl_INCLUDE_DIRS} CACHE STRING "Which nytl to use")
		set(evg_build off)
	else()
		message("[config] Could not find evg and nytl. Will build my own evg version")
		set(evg_location "own" CACHE STRING "Which evg to use")
		set(nytl_location "own" CACHE STRING "Which nytl to use")
		set(evg_build on)
	endif()
endif()

#configure vpp
if(WithVulkan AND NOT vpp_location)
	find_package(vpp 0.1.0 QUIET)

	if(vpp_FOUND)
		message("[config] Found vpp headers at " ${vpp_INCLUDE_DIRS})
		set(vpp_location ${vpp_INCLUDE_DIRS} CACHE STRING "Which vpp to use")
		set(vpp_build off)
	else()
		message("[config] Could not find vpp. Will build my own vpp version")
		set(vpp_location "own" CACHE STRING "Which vpp to use")
		set(vpp_build on)
	endif()
endif()

#config file
#first set all macros (they have NY_ prefix since they are macros)
set(NY_WithGL ${WithGL})
set(NY_WithEGL ${WithEGL})
set(NY_WithVulkan ${WithVulkan})
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

message("\tnytl build:\t\t" ${nytl_location})
message("\tevg build:\t\t" ${evg_location})

if(WithVulkan)
	message("\tvpp build:\t\t" ${vpp_location})
endif()

message("\n\tny version:\t\t" ${vmajor} "." ${vminor} "." ${vpatch})
message("\n")

# contributions to the ny cmake files are highly appreciated. lol
