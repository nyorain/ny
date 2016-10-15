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
class NativeWindowHandle;
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

//integration
class Surface;
class BufferSurface;
class BufferGuard;

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
