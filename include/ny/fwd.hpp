#pragma once

#include <cstdint>

//This file contains forward delcarations for all ny classes and enumerations.
//Can be useful if one does not want to pull in any ny headers but needs many of its
//classes (used e.g. as pointers or references).

//evg forward declarations
//github.com/nyorain/evg
namespace evg
{

class DrawContext;
class DrawGuard;
class UniqueDrawGuard;
class Image;
class SvgImage;
class AnimatedImage;
class Shape;
class Brush;
class Pen;
class Font;
class Texture;
class Path;
class PathBase;
class Rectangle;
class Circle;
class Subpath;
class PlainSubpath;
class PathSegment;
class Color;

enum class ImageFormat : unsigned int;

}

//nytl forward delcarations.
//github.com/nyroain/nytl
namespace nytl
{

class Logger;
class StringParam;

template<std::size_t I, typename T> class Vec;
template<std::size_t R, std::size_t C, typename P> class Mat;
template<typename Signature> class Callback;
template<typename T> class Range;

template<typename T> using Vec2 = Vec<2, T>;
template<typename T> using Vec3 = Vec<3, T>;
template<typename T> using Vec4 = Vec<4, T>;

using Vec2ui = Vec2<unsigned int>;
using Vec2i = Vec2<int>;
using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;

using Vec3ui = Vec3<unsigned int>;
using Vec3i = Vec3<int>;
using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;

}

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
class ImageData;

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
class DrawEvent;
class ShowEvent;
class CloseEvent;

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
