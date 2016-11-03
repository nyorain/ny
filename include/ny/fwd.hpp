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

//base
class Event;
class EventHandler;
class EventDispatcher;
class DefaultEventDispatcher;
class ThreadedEventDispatcher;
class Cursor;
class LoopControl;
class LoopControlImpl;
class DataTypes;
class DataOffer;
class DataSource;
class DataObject;

template<typename P> struct BasicImageData;
using ImageData = BasicImageData<const std::uint8_t*>;
using MutableImageData = BasicImageData<std::uint8_t*>;

//backend
class Backend;
class WindowContext;
class AppContext;
class DialogContext;
class MouseContext;
class KeyboardContext;
class WindowSettings;
class NativeHandle;
class Keyboard;
class Mouse;

//events
class FocusEvent;
class MouseMoveEvent;
class MouseButtonEvent;
class MouseCrossEvent;
class MouseWheelEvent;
class SizeEvent;
class PositionEvent;
class KeyEvent;
class RefreshEvent;
class ShowEvent;

//integration, contexts
class Surface;
class BufferSurface;
class BufferGuard;

using GlConfigId = struct GlConfigIdType*; //opaque
class GlSetup;
class GlContext;
class GlSurface;

struct GlVersion;
struct GlConfig;

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
enum class ContextType : unsigned int;
enum class CursorType : unsigned int;
enum class ImageDataFormat : unsigned int;

}

//XXX: duh... please just make this go away...
#define DEFINE_HANDLE(object) typedef struct object##_T* object;

#if defined(__LP64__) || \
	defined(_WIN64) || \
	(defined(__x86_64__) && !defined(__ILP32__)) || \
	defined(_M_X64) || \
	defined(__ia64) || \
	defined (_M_IA64) || \
	defined(__aarch64__) || \
	defined(__powerpc64__)
	#define DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
#else
	#define DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef std::uint64_t object;
#endif

DEFINE_HANDLE(VkInstance);
DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR);

#undef DEFINE_NON_DISPATCHABLE_HANDLE
#undef DEFINE_HANDLE
