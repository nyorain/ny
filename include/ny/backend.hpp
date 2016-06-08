#pragma once

#include <ny/include.hpp>
#include <ny/backend/backend.hpp>
#include <ny/backend/windowContext.hpp>
#include <ny/backend/appContext.hpp>

#if NY_WithWinapi
 #include <ny/backend/winapi/windowContext.hpp>
 #include <ny/backend/winapi/appContext.hpp>
#endif //Winapi

#if NY_WithX11
 #include <ny/backend/x11/windowContext.hpp>
 #include <ny/backend/x11/appContext.hpp>
#endif //X11

#if NY_WithWayland
 #include <ny/backend/wayland/windowContext.hpp>
 #include <ny/backend/wayland/appContext.hpp>
#endif
