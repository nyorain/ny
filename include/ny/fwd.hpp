#pragma once

namespace ny
{

class Window;
class ToplevelWindow;
class ChildWindow;
class WindowSettings;
class App;
class WindowContext;
class AppContext;
class DrawContext;
class DrawGuard;
class Image;
class AnimatedImage;
class SvgImage;
class SvgDrawContext;
class Event;
class EventHandler;
class EventDispatcher;
class DefaultEventDispatcher;
class ThreadedEventDispatcher;
class Gui;
class Widget;
class WidgetBase;
class Button;
class Textfield;
class Checkbox;
class Dropdown;
class MenuWidget;
class NativeWidget;
class Label;
class Backend;
class Font;
class Shape;
class Path;
class PathSegment;
class Circle;
class Rectangle;
class Text;
class Subpath;
class PlainSubpath;
class Brush;
class Pen;
class DataTypes;
class DataOffer;
class DataSource;
class Cursor;
class Frame;
class Dialog;
class MessageBox;
class Popup;
class Texture;
class LoopControl;
class LoopControlImpl;

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
class ContextEvent;

class Keyboard;
class Mouse;

class GlContext;
class GlResource;
class GlDrawContext;
class GlVertexArray;
class GlBuffer;
class Shader;
class FreeTypeFontHandle;
class CairoFontHandle;
class CairoDrawContext;
class RedirectDrawContext;
class DelayedDrawContext;

enum class ToplevelState : unsigned char;
enum class Preference : unsigned char;
enum class NativeWidgetType : unsigned char;
enum class WindowEdge : unsigned char;
enum class WindowHint : unsigned int;

}

