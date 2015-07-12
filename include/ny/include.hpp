#pragma once

#include <ny/config.h>
#include <cstdlib>

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
class toplevelWindowContext;
class childWindowContext;
class windowContextSettings;

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

class file;

enum class preference : unsigned char;
enum class windowEdge : unsigned char;
enum class bufferFormat : unsigned char;

class drawContext;
class cairoDrawContext;
class gdiDrawContext;

class mask;
class shape;
class path;

template<size_t dim, class prec = float> class transformable;
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

typedef cairoDrawContext cairoDC;
typedef gdiDrawContext gdiDC;

typedef windowContext wc;
typedef toplevelWindowContext toplevelWC;
typedef childWindowContext childWC;

typedef appContext ac;

//todo: fix NY_With*
class freeTypeFont;
class cairoFont;
class gdiFont;

#ifdef NY_WithGL
class shader;

class glContext;

class legacyGLContext;
class modernGLContext;

class glDrawContext;

typedef glDrawContext glDC;
typedef glContext glc;
#endif // NY_WithGL

//utils
template<size_t rows, size_t cols, class prec> class mat;
template<size_t dim, class T> class vec;
template<size_t dim, class T> class refVec;
template<size_t dim, class prec> class rect;

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

}
