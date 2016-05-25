#pragma once

#include <ny/include.hpp>
#include <ny/draw/image.hpp>
#include <ny/draw/shape.hpp>
#include <ny/draw/color.hpp>
#include <ny/draw/brush.hpp>
#include <ny/draw/gradient.hpp>
#include <ny/draw/pen.hpp>
#include <ny/draw/font.hpp>
#include <ny/draw/drawContext.hpp>

#ifdef NY_WithGL
 #include <ny/draw/gl/context.hpp>
 #include <ny/draw/gl/drawContext.hpp>
 #include <ny/draw/gl/resource.hpp>
#endif //GL

#ifdef NY_WithEGL
 #include <ny/draw/gl/egl.hpp>
#endif //EGL

#ifdef NY_WithWinapi
 #include <ny/draw/gdi.hpp>
#endif //Winapi

#ifdef NY_WithCairo
 #include <ny/draw/cairo.hpp>
#endif //Cairo

#ifdef NY_WithFreeType
 #include <ny/draw/freetype.hpp>
#endif //Freetype
