#pragma once

#include <ny/include.hpp>
#include <ny/window/cursor.hpp>
#include <ny/window/windowEvents.hpp>
#include <ny/window/windowDefs.hpp>
#include <ny/app/data.hpp>
#include <ny/app/eventHandler.hpp>

#include <nytl/callback.hpp>
#include <nytl/vec.hpp>

#include <memory>
#include <functional>

namespace ny
{

//todo: events masks for windows
//todo: implement all callback-add-functions for window correctly. can it be done with some template-like method?

//window class
class Window : public EventHandler
{
protected:
    //position and max/min - size. size itself inherited from surface
    vec2i position_ {0, 0};
    vec2ui size_ {0, 0};

    vec2ui minSize_ {0, 0};
    vec2ui maxSize_ {1 << 30, 1 << 30};

    //states saved in window, not in the context
    bool focus_ {0};
    bool mouseOver_ {0};
    bool shown_ {0};

    //current window cursor
    Cursor cursor_;

    //windowContext. backend specific object. Core element of window. 
	//window itself is not able to change anything, it has to communicate with the backend 
	//through this object
    std::unique_ptr<WindowContext> windowContext_;

    //stores window hints. most of window hints (e.g. Native<> or Maximize/Minimize etc. are not 
	//set by window class itself, but by the specific derivations of window
    //do NOT represent window states (as focus_, mouseOver_ etc) but general information 
	//about the window, which should not be changed by the window or its context after 
	//creation (of course they can be changed from the outside)
    unsigned long hints_ {0};

    //files of the types listed in the dataType (basically vector of dataTypes) will 
	//generate a dataReceiveEvent when they are dropped on the window
    DataTypes dropAccept_ {};


    //callbacks
    callback<void(Window&, DrawContext&)> drawCallback_;
    callback<void(Window&, const vec2ui&)> resizeCallback_;
    callback<void(Window&, const vec2i&)> moveCallback_;
    callback<void(Window&)> destroyCallback_;
    callback<void(Window&, const FocusEvent&)> focusCallback_;
    callback<void(Window&, const ShowEvent&)> showCallback_;

    callback<void(Window&, const MouseMoveEvent&)> mouseMoveCallback_;
    callback<void(Window&, const MouseButtonEvent&)> mouseButtonCallback_;
    callback<void(Window&, const MouseCrossEvent&)> mouseCrossCallback_;
    callback<void(Window&, const MouseWheelEvent&)> mouseWheelCallback_;

    callback<void(Window&, const KeyEvent&)> keyCallback_;

    //events - have to be protected?
    virtual void mouseMove(const MouseMoveEvent&);
    virtual void mouseCross(const MouseCrossEvent&);
    virtual void mouseButton(const MouseButtonEvent&);
    virtual void mouseWheel(const MouseWheelEvent&);
    virtual void keyboardKey(const KeyEvent&);
    virtual void windowSize(const SizeEvent&);
    virtual void windowPosition(const PositionEvent&);
    virtual void windowDraw(const DrawEvent&);
    virtual void windowShow(const ShowEvent&);
    virtual void windowFocus(const FocusEvent&);

    //windowContext functions are protected, derived classes can make public aliases if needed. 
	//User should not be able to change backend Hints for button e.g
    void cursor(const Cursor& curs);

    void setBackendHints(unsigned int backend, unsigned long hints);
    void addBackendHints(unsigned int backend, unsigned long hints);
    void removeBackendHints(unsigned int backend, unsigned long hints);

    void setWindowHints(unsigned long hints);
    void addWindowHints(unsigned long hints);
    void removeWindowHints(unsigned int hints);

    void acceptedDropTypes(const DataTypes& d);
    void addDropType(unsigned char type);
    void removeDropType(unsigned char type);

    virtual void draw(DrawContext& dc);


    //window is abstract class
    Window() = default;
    Window(const vec2ui& size, const WindowContextSettings& settings = {});

    void create(const vec2ui& size, const WindowContextSettings& settings = {});

public:
    virtual ~Window();

    virtual bool processEvent(const Event& event) override;
	virtual void close();

    virtual ToplevelWindow* toplevelParent() = 0;
    virtual const ToplevelWindow* toplevelParent() const = 0;

    void size(const vec2ui& size);
    void position(const vec2i& position);

    //std::vector<ChildWindow*> getWindowChildren();
    //window* getWindowAt(vec2i position);

    void move(const vec2i& delta);

    void minSize(const vec2ui& size);
    void maxSize(const vec2ui& size);

    void refresh();

    void show();
    void hide();
    void toggleShow();

    //callbacks
    template<typename F> connection onDraw(F&& func){ return drawCallback_.add(func); }
    template<typename F> connection onResize(F&& func){ return resizeCallback_.add(func); }
    template<typename F> connection onMove(F&& func){ return moveCallback_.add(func); }
    template<typename F> connection onDestroy(F&& func){ return destroyCallback_.add(func); }
    template<typename F> connection onFocus(F&& func){ return focusCallback_.add(func); }
    template<typename F> connection onShow(F&& func){ return showCallback_.add(func); }
    template<typename F> connection onMouseMove(F&& func){ return mouseMoveCallback_.add(func); }
    template<typename F> connection onMouseButton(F&& func){return mouseButtonCallback_.add(func);}
    template<typename F> connection onMouseCross(F&& func){ return mouseCrossCallback_.add(func); }
    template<typename F> connection onMouseWheel(F&& func){ return mouseWheelCallback_.add(func); }
    template<typename F> connection onKey(F&& func){ return keyCallback_.add(func); }

    //const getters
    const vec2i& position() const { return position_; }
    const vec2ui& size() const { return size_; }

    const vec2ui& minSize() const { return minSize_; }
    const vec2ui& maxSize() const { return maxSize_; }

    bool focus() const { return focus_; }
    bool mouseOver() const { return mouseOver_; }
    bool shown() const { return shown_; }

    unsigned long windowHints() const { return hints_; }
    WindowContext* windowContext() const { return windowContext_.get(); }

    const Cursor& cursor() const { return cursor_; }

    DataTypes acceptedDropTypes() const { return dropAccept_; }
    bool dropTypeAccepted(unsigned char type) const { return dropAccept_.contains(type); }

    WindowContextSettings windowContextSettings() const;
    unsigned long backendHints() const;
};

}

