#pragma once

#include <ny/include.hpp>
#include <ny/base/image.hpp>
#include <ny/draw/shape.hpp>
#include <ny/draw/color.hpp>
#include <ny/draw/brush.hpp>
#include <ny/draw/gradient.hpp>
#include <ny/draw/pen.hpp>
#include <ny/draw/font.hpp>
#include <ny/draw/drawContext.hpp>

#if NY_WithGL
 #include <ny/draw/gl/context.hpp>
 #include <ny/draw/gl/drawContext.hpp>
 #include <ny/draw/gl/resource.hpp>
#endif //GL

#if NY_WithEGL
 #include <ny/draw/gl/egl.hpp>
#endif //EGL

#if NY_WithWinapi
 #include <ny/draw/gdi.hpp>
#endif //Winapi

#if NY_WithCairo
 #include <ny/draw/cairo.hpp>
#endif //Cairo

#if NY_WithFreeType
 #include <ny/draw/freeType.hpp>
#endif //Freetype
