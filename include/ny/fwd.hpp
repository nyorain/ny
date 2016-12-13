// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <cstdint>
#include <cstdlib>
#include <nytl/fwd.hpp>

//This file contains forward delcarations for all ny classes and enumerations.
//Can be useful if one does not want to pull in any ny headers but needs many of its
//classes (used e.g. as pointers, references or template parameters (in some cases ok)).

//own forward declarations
namespace ny
{

using namespace nytl;

//treat them as built-in
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;

class Cursor;
class LoopControl;
class LoopInterface;
class DataFormat;
class DataOffer;
class DataSource;

class Backend;
class WindowContext;
class AppContext;
class DialogContext;
class MouseContext;
class KeyboardContext;
class WindowSettings;
class WindowListener;
class EventData;
class NativeHandle;
class Keyboard;
class Mouse;

class Surface;
class BufferSurface;
class BufferGuard;

using GlConfigID = struct GlConfigIDType*; //opaque, see common/gl.hpp
class GlSetup;
class GlContext;
class GlSurface;

struct GlVersion;
struct GlConfig;

class EglSetup;
class EglSurface;
class EglContext;

template<typename P> class BasicImage;
using Image = BasicImage<const uint8_t*>;
using MutableImage = BasicImage<uint8_t*>;

//enums
enum class ToplevelState : unsigned int;
enum class WindowEdge : unsigned int;
enum class WindowHint : unsigned int;
enum class WindowCapability : unsigned int;
enum class Keycode : unsigned int;
enum class MouseButton : unsigned int;
enum class DialogResult : unsigned int;
enum class NativeWidgetType : unsigned int;
enum class DialogType : unsigned int;
enum class CursorType : unsigned int;
enum class ColorChannel : uint8_t;

using WindowHints = nytl::Flags<WindowHint>;
using WindowEdges = nytl::Flags<WindowEdge>;
using WindowCapabilities = nytl::Flags<WindowCapability>;

}

using VkAllocationCallbacks = struct VkAllocationCallbacks;
using VkInstance = struct VkInstance_T*;
