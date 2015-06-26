# Install script for directory: /mnt/sda5/Programming/projects/libny-1/include/ny

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/mnt/sda5/Programming/projects/libny-1/build/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ny" TYPE FILE FILES
    "/mnt/sda5/Programming/projects/libny-1/include/ny/config.h"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/ny.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/include.hpp"
    )
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ny/app" TYPE FILE FILES
    "/mnt/sda5/Programming/projects/libny-1/include/ny/app/app.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/app/cursor.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/app/dnd.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/app/error.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/app/event.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/app/eventHandler.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/app/file.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/app/keyboard.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/app/mouse.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/app/surface.hpp"
    )
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ny/window" TYPE FILE FILES
    "/mnt/sda5/Programming/projects/libny-1/include/ny/window/window.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/window/frame.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/window/widget.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/window/widgets.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/window/dialog.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/window/style.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/window/windowDefs.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/window/windowEvents.hpp"
    )
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ny/utils" TYPE FILE FILES
    "/mnt/sda5/Programming/projects/libny-1/include/ny/utils/vec.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/utils/mat.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/utils/rect.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/utils/callback.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/utils/thread.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/utils/time.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/utils/region.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/utils/proto.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/utils/misc.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/utils/animation.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/utils/refVec.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/utils/nonCopyable.hpp"
    )
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ny/backends" TYPE FILE FILES
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/backend.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/appContext.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/windowContext.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/x11/appContext.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/x11/backend.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/x11/cairo.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/x11/defs.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/x11/utils.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/x11/windowContext.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/x11/egl.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/x11/glx.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/wayland/appContext.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/wayland/backend.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/wayland/cairo.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/wayland/defs.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/wayland/utils.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/wayland/windowContext.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/backends/wayland/gl.hpp"
    )
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ny/graphics" TYPE FILE FILES
    "/mnt/sda5/Programming/projects/libny-1/include/ny/graphics/drawContext.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/graphics/shape.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/graphics/color.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/graphics/cairo.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/graphics/font.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/graphics/freeType.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/graphics/image.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/graphics/texture.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/graphics/transformable.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/graphics/gl/glContext.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/graphics/gl/glDC.hpp"
    "/mnt/sda5/Programming/projects/libny-1/include/ny/graphics/gl/shader.hpp"
    )
endif()

