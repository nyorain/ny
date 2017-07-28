// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#define UNICODE

#include <windows.h>
#include <winuser.h>
#include <windowsx.h>

// undefine the shittiest macros
// holy fuck microsoft...
#undef near
#undef far
#undef ERROR
#undef MemoryBarrier
#undef UNICODE
