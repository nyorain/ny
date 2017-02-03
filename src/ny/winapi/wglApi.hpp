// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/winapi/include.hpp>
#include <ny/winapi/windows.hpp>
#include <Wingdi.h> // static wgl functions

namespace ny {
namespace {

// Extension functions typedefs
using PfnWglCreateContextAttribsARB = HGLRC (*)(HDC, HGLRC, const int*);
using PfnWglSwapIntervalEXT = BOOL (*)(int);
using PfnWglGetExtensionStringARB = const char* (*)(HDC);
using PfnWglGetPixelFormatAttribivARB = BOOL(*)(HDC, int, int, UINT, const int*, int*);
using PfnWglChoosePixelFormatARB =  BOOL (*)(HDC, const int*,
	const FLOAT*, UINT, int*, UINT*);

// Extension function pointers
// loaded exactly once in a threadsafe manner
PfnWglGetExtensionStringARB getExtensionStringARB {};
PfnWglChoosePixelFormatARB choosePixelFormatARB {};
PfnWglGetPixelFormatAttribivARB getPixelFormatAttribivARB {};
PfnWglCreateContextAttribsARB createContextAttribsARB {};
PfnWglSwapIntervalEXT swapIntervalEXT {};

// all supported extensions string
const char* extensions {};

// bool flags only for extensions that have to functions pointers
bool hasMultisample {};
bool hasSwapControlTear {};
bool hasProfile {};
bool hasProfileES {};

#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
	constexpr auto WGL_CONTEXT_DEBUG_BIT_ARB = 0x00000001;
	constexpr auto WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB = 0x00000002;
	constexpr auto WGL_CONTEXT_ES2_PROFILE_BIT_EXT = 0x00000004;
	constexpr auto WGL_CONTEXT_MAJOR_VERSION_ARB = 0x2091;
	constexpr auto WGL_CONTEXT_MINOR_VERSION_ARB = 0x2092;
	constexpr auto WGL_CONTEXT_LAYER_PLANE_ARB = 0x2093;
	constexpr auto WGL_CONTEXT_FLAGS_ARB = 0x2094;
	constexpr auto ERROR_INVALID_VERSION_ARB = 0x2095;
	constexpr auto WGL_CONTEXT_PROFILE_MASK_ARB = 0x9126;
	constexpr auto WGL_CONTEXT_CORE_PROFILE_BIT_ARB = 0x00000001;
	constexpr auto WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB = 0x00000002;
	constexpr auto ERROR_INVALID_PROFILE_ARB = 0x2096;
	constexpr auto WGL_SAMPLE_BUFFERS_ARB = 0x2041;
	constexpr auto WGL_SAMPLES_ARB = 0x2042;
	constexpr auto WGL_NUMBER_PIXEL_FORMATS_ARB = 0x2000;
	constexpr auto WGL_DRAW_TO_WINDOW_ARB = 0x2001;
	constexpr auto WGL_DRAW_TO_BITMAP_ARB = 0x2002;
	constexpr auto WGL_ACCELERATION_ARB = 0x2003;
	constexpr auto WGL_TRANSPARENT_ARB = 0x200A;
	constexpr auto WGL_SHARE_DEPTH_ARB = 0x200C;
	constexpr auto WGL_SHARE_STENCIL_ARB = 0x200D;
	constexpr auto WGL_SHARE_ACCUM_ARB = 0x200E;
	constexpr auto WGL_SUPPORT_GDI_ARB = 0x200F;
	constexpr auto WGL_SUPPORT_OPENGL_ARB = 0x2010;
	constexpr auto WGL_DOUBLE_BUFFER_ARB = 0x2011;
	constexpr auto WGL_STEREO_ARB = 0x2012;
	constexpr auto WGL_PIXEL_TYPE_ARB = 0x2013;
	constexpr auto WGL_COLOR_BITS_ARB = 0x2014;
	constexpr auto WGL_RED_BITS_ARB = 0x2015;
	constexpr auto WGL_RED_SHIFT_ARB = 0x2017;
	constexpr auto WGL_GREEN_BITS_ARB = 0x2017;
	constexpr auto WGL_GREEN_SHIFT_ARB = 0x2018;
	constexpr auto WGL_BLUE_BITS_ARB = 0x2019;
	constexpr auto WGL_BLUE_SHIFT_ARB = 0x201A;
	constexpr auto WGL_ALPHA_BITS_ARB = 0x201B;
	constexpr auto WGL_ALPHA_SHIFT_ARB = 0x201C;
	constexpr auto WGL_DEPTH_BITS_ARB = 0x2022;
	constexpr auto WGL_STENCIL_BITS_ARB = 0x2023;
	constexpr auto WGL_AUX_BUFFERS_ARB = 0x2024;
	constexpr auto WGL_NO_ACCELERATION_ARB = 0x2025;
	constexpr auto WGL_GENERIC_ACCELERATION_ARB = 0x2026;
	constexpr auto WGL_FULL_ACCELERATION_ARB = 0x2027;
	constexpr auto WGL_TYPE_RGBA_ARB = 0x202B;
#endif

} // anonymous util namespace
} // namespace ny
