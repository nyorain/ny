cmake_minimum_required (VERSION 3.1.0)

project(ny)

set(NY_VMAJOR 0)
set(NY_VMINOR 1)

set(CMAKE_BUILD_TYPE Debug)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/src")

include_directories("${CMAKE_CURRENT_BINARY_DIR}/external/install/include")
link_directories("${CMAKE_CURRENT_BINARY_DIR}/external/install/lib")
link_directories("${CMAKE_CURRENT_BINARY_DIR}/external/install/bin")

option(NY_BUILD_EXAMPLES "build the ny examples" ON)
option(NY_BUILD_TESTS "build the ny tests" OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

#todo
set(NY_WithGL 0)
set(NY_WithEGL 0)
set(NY_WithFreeType 0)
set(NY_WithCairo 0)
#set(NY_WithX11 1)
set(NY_WithWinapi 1)


#config, TODO: install
configure_file	( "${CMAKE_CURRENT_SOURCE_DIR}/src/ny/config.hpp.in"
	"${CMAKE_CURRENT_BINARY_DIR}/include/ny/config.hpp")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/ny/config.hpp" DESTINATION include/ny)

add_subdirectory(src/ny)
add_subdirectory(include/ny)
add_subdirectory(external)

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
