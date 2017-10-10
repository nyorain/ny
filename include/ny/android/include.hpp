// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <ny/config.hpp>

#ifndef NY_WithAndroid
	#error ny was built without android. Do not include this header file!
#endif //Android

// android fwd decl
using AInputEvent = struct AInputEvent;
using ANativeWindow = struct ANativeWindow;
using ALooper = struct ALooper;
using AInputQueue = struct AInputQueue;

struct ANativeWindow_Buffer;
struct ANativeActivity;

// jni fwd decl
using JNIEnv = struct _JNIEnv;
using JavaVM = struct _JavaVM;

using jclass = class _jclass*;
using jmethodID = struct _jmethodID*;

namespace ny {

class AndroidBackend;
class AndroidAppContext;
class AndroidWindowContext;
class AndroidBufferSurface;
class AndroidMouseContext;
class AndroidKeyboardContext;

namespace android {
class Activity;
struct ActivityEvent;	
}

} // namespace ny
