#pragma once

namespace nyutil
{
}

namespace ny
{
using namespace nyutil;
}

#include <ny/config.h>
#include <ny/error.hpp> //to make nyDebug available everywhere for the moment. remove later
#include <iostream> //same
#include <cstdlib>

namespace nyutil
{
template<size_t rows, size_t cols, class prec> class mat;
template<size_t dim, class T> class vec;
template<size_t dim, class T> class refVec;
template<size_t dim, class prec> class rect;

template<size_t dim, typename prec> class transformable;
using transformable2 = transformable<2, float>;
using transformable3 = transformable<3, float>;

template < class > class callback;
class connection;
class callbackBase;

class timeDuration;
class timePoint;
class timer;

class region;
class threadpool;
class task;
class threadSafeObj;
class nonCopyable;

class eventLoop;
class eventSource;
class pollEventSource;

}

namespace ny
{

class backend;
class app;
class eventHandler;
class color;
class style;
class cursor;
class image;

class window;
class toplevelWindow;
class childWindow;

class keyboard;
class mouse;
//class touch;

class appContext;
class windowContext;
class windowContextSettings;
class drawContext;

class dataTypes;
class dataOffer;
class dataSource;

class event;
class focusEvent;
class keyEvent;
class mouseMoveEvent;
class mouseCrossEvent;
class mouseMoveEvent;
class mouseWheelEvent;
class mouseButtonEvent;
class sizeEvent;
class drawEvent;
class positionEvent;
class showEvent;
class destroyEvent;
class contextEvent;

class button;
class headerbar;
class textfield;
class panel;

class file;

enum class preference : unsigned char;
enum class windowEdge : unsigned char;
enum class bufferFormat : unsigned char;

class mask;
class shape;
class path;

class redirectDrawContext;

class surface;
class font;
class pointArray;

class rectangle;
class circle;
class text;
class customPath;

//todo
typedef color brush;
typedef color pen;
//class brush;
//class pen;

typedef drawContext dc;
typedef windowContext wc;
typedef appContext ac;

#ifdef NY_WithFreeType
class freeTypeFont;
#endif //FreeType

#ifdef NY_WithWinapi
class gdiFont;
class gdiDrawContext;
typedef gdiDrawContext gdiDC;
#endif //Winapi

#ifdef NY_WithCairo
class cairoFont;
class cairoDrawContext;
typedef cairoDrawContext cairoDC;
#endif //Cairo

#ifdef NY_WithGL
class shader;
class glDrawContext;
typedef glDrawContext glDC;

#ifdef NY_WithEGL
class eglDrawContext;
class eglAppContext;

typedef eglAppContext eglAC;
#endif // NY_WithEGL

#endif // NY_WithGL

//utils



}

