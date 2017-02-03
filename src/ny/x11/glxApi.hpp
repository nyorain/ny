// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/include.hpp>
#include <GL/glx.h> // static glx functions

namespace ny {
namespace {

using PfnGlxSwapIntervalEXT = void (APIENTRYP)(Display*, GLXDrawable, int);
using PfnGlxCreateContextAttribsARB = GLXContext(APIENTRYP)(Display*, GLXFBConfig,
	GLXContext, Bool, const int*);

// extensions function pointers
PfnGlxSwapIntervalEXT swapIntervalEXT {};
PfnGlxCreateContextAttribsARB createContextAttribsARB {};

// bool flags for extensions without functions
bool hasSwapControlTear {};
bool hasProfile {};
bool hasProfileES {};

constexpr auto GLX_CONTEXT_DEBUG_BIT_ARB = 0x00000001;
constexpr auto GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB = 0x00000002;
constexpr auto GLX_CONTEXT_MAJOR_VERSION_ARB = 0x2091;
constexpr auto GLX_CONTEXT_MINOR_VERSION_ARB = 0x2092;
constexpr auto GLX_CONTEXT_FLAGS_ARB = 0x2094;
constexpr auto GLX_CONTEXT_CORE_PROFILE_BIT_ARB = 0x00000001;
constexpr auto GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB = 0x00000002;
constexpr auto GLX_CONTEXT_PROFILE_MASK_ARB = 0x9126;
constexpr auto GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB = 0x20B2;
constexpr auto GLX_SAMPLE_BUFFERS_ARB = 100000;
constexpr auto GLX_SAMPLES_ARB = 100001;
constexpr auto GLX_CONTEXT_ES2_PROFILE_BIT_EXT = 0x00000004;
constexpr auto GLX_CONTEXT_ES_PROFILE_BIT_EXT = 0x00000004;
constexpr auto GLX_SWAP_INTERVAL_EXT = 0x20F1;
constexpr auto GLX_MAX_SWAP_INTERVAL_EXT = 0x20F2;

} // anonymous util namespace
} // namespace ny
