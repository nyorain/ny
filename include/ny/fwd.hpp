#pragma once

//This file contains forward delcarations for all ny classes and enumerations.
//Can be useful if one does not want to pull in any ny headers but needs many of its
//classes (used e.g. as pointers or references).

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
class DrawEvent;
class ShowEvent;
class CloseEvent;

//enums
enum class ToplevelState : unsigned char;
enum class Preference : unsigned char;
enum class NativeWidgetType : unsigned char;
enum class WindowEdge : unsigned char;
enum class WindowHint : unsigned int;
enum class Key : unsigned int;
enum class MouseButton : unsigned int;

}
