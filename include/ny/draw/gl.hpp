#pragma once

#include <ny/draw/include.hpp>

#ifdef NY_WithGL

#include <ny/draw/gl/context.hpp>
#include <ny/draw/gl/drawContext.hpp>
#include <ny/draw/gl/shader.hpp>

#ifdef NY_WithEGL
#include <ny/draw/gl/egl.hpp>
#endif //WithEGL

#ifdef NY_WithGLES
#include <ny/draw/gl/gles.hpp>
#endif //WithGLES

#endif //WithGL
