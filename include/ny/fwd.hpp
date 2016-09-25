#pragma once

#include <cstdint>
#include <cstdlib>
#include <nytl/fwd.hpp>

//This file contains forward delcarations for all ny classes and enumerations.
//Can be useful if one does not want to pull in any ny headers but needs many of its
//classes (used e.g. as pointers or references).

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

template<typename P> class BasicImageData;
using ImageData = BasicImageData<const std::uint8_t*>;
using MutableImageData = BasicImageData<std::uint8_t*>;

//app
class App;
class Window;
class ToplevelWindow;
class ChildWindow;
class Label;
class Keyboard;
class Mouse;
class Frame;
class Dialog;
class MessageBox;

//backend
class Backend;
class WindowContext;
class AppContext;
class DialogContext;
class MouseContext;
class KeyboardContext;
class WindowSettings;
class GlContext;
class VulkanContext;
class VulkanSurfaceContext;
class NativeWindowHandle;

//gui
class Gui;
class Widget;
class WidgetBase;
class Button;
class Textfield;
class Checkbox;
class Dropdown;
class MenuWidget;
class NativeWidget;

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
// class DrawEvent;
class ShowEvent;
// class CloseEvent;

//enums
enum class ToplevelState : unsigned int;
enum class Preference : unsigned int;
enum class NativeWidgetType : unsigned int;
enum class WindowEdge : unsigned int;
enum class WindowHint : unsigned int;
enum class WindowCapability : unsigned int;
enum class Key : unsigned int;
enum class MouseButton : unsigned int;
enum class DialogResult : unsigned int;
enum class DialogType : unsigned int;
enum class DrawState : unsigned int;
enum class CursorType : unsigned int;

}
