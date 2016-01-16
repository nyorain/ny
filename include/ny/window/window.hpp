#pragma once

#include <ny/window/include.hpp>
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
    std::unique_ptr<WindowContext> windowContext_ {nullptr};

    //stores window hints. most of window hints (e.g. Native<> or Maximize/Minimize etc. are not 
	//set by window class itself, but by the specific derivations of window
    //do NOT represent window states (as focus_, mouseOver_ etc) but general information 
	//about the window, which should not be changed by the window or its context after 
	//creation (of course they can be changed from the outside)
    unsigned long hints_ {0};

    //files of the types listed in the dataType (basically vector of dataTypes) will 
	//generate a dataReceiveEvent when they are dropped on the window
    dataTypes dropAccept_ {};


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

    void setAcceptedDropTypes(const dataTypes& d);
    void addDropType(unsigned char type);
    void removeDropType(unsigned char type);

    virtual void draw(DrawContext& dc);

    //window is abstract class
    Window();
    Window(const vec2ui& size, const WindowContextSettings& settings = {});
    void create(const vec2ui& size, const WindowContextSettings& settings = {});

public:
    virtual ~Window();

    virtual bool processEvent(const Event& event) override;
	virtual void destroy();

    virtual Window* parent() const { return nullptr; }

    virtual ToplevelWindow* topLevelParent() = 0;
    virtual const ToplevelWindow* topLevelParent() const = 0;

    virtual bool isVirtual() const { return 0; }

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
    const vec2ui& getSize() const { return size_; }

    const vec2ui& minSize() const                           { return minSize_; }
    const vec2ui& maxSize() const                           { return maxSize_; }

    bool focus() const                               { return focus_; }
    bool mouseOver() const                           { return mouseOver_; }
    bool shown() const                               { return shown_; }

    unsigned long windowHints() const                { return hints_; }
    WindowContext* windowContext() const             { return windowContext_.get(); }

    const Cursor& cursor() const                     { return cursor_; }

    dataTypes getAcceptedDropTypes() const              { return dropAccept_; }
    bool dropTypeAccepted(unsigned char type) const     { return dropAccept_.contains(type); }

    //get data from windowContext, have to check if window is valid. defined in window.cpp
    WindowContextSettings getWindowContextSettings() const;
    unsigned long backendHints() const;
};

/*
//toplevel window
class ToplevelWindow : public Window
{
protected:
    //hints here not really needed
    //only for virtual handling, but they all are still saved in windowContext. rather make alias functions to access the on the WC stored hints
    unsigned char handlingHints_{};
    toplevelState state_{};
    std::string title_{};
    unsigned int borderSize_{};

    //these both are used (and only valid) for custom decoration
    std::unique_ptr<headerbar> headerbar_;
    std::unique_ptr<panel> panel_;

    //evthandler
    //virtual void addChild(eventHandlerNode& window) override;

    //window
    virtual void mouseButton(const mouseButtonEvent& ev) override;
    virtual void mouseMove(const mouseMoveEvent& ev) override;

    //draw window
    virtual void draw(drawContext& dc) override;

    toplevelWindow();
    void create(vec2i position, vec2ui size, std::string title = " ", const windowContextSettings& settings = windowContextSettings());

public:
    toplevelWindow(vec2i position, vec2ui size, std::string title = " ", const windowContextSettings& settings = windowContextSettings());
    virtual ~toplevelWindow();


    //hints
    bool hasMaximizeHint() const { return (hints_ & windowHints::Maximize); }
    bool hasMinimizeHint() const { return (hints_ & windowHints::Minimize); }
    bool hasResizeHint() const { return (hints_ & windowHints::Resize); }
    bool hasMoveHint() const { return (hints_ & windowHints::Move); }
    bool hasCloseHint() const { return (hints_ & windowHints::Close); }

    void setMaximizeHint(bool hint = 1);
    void setMinimizeHint(bool hint = 1);
    void setResizeHint(bool hint = 1);
    void setMoveHint(bool hint = 1);
    void setCloseHint(bool hint = 1);

//
    bool isCustomDecorated() const {  return (hints_ & windowHints::CustomDecorated); }
    bool isCustomMoved() const { return (hints_ & windowHints::CustomMoved); }
    bool isCustomResized() const { return (hints_ & windowHints::CustomResized); }

    //return if successful
    bool setCustomDecorated(bool set = 1);
    bool setCustomMoved(bool set = 1);
    bool setCustomResized(bool set = 1);
//
    ////
    std::string getTitle() const { return title_; }
    void setTitle(const std::string& n);

    void setIcon(const image* icon);

    const toplevelWindow* getTopLevelParent() const { return this; };
    toplevelWindow* getTopLevelParent() { return this; };

    window* getParent() const { return nullptr; };

    bool isMaximized() const { return (state_ == toplevelState::Maximized); };
    bool isMinimized() const { return (state_ == toplevelState::Minimized); };
    bool isFullscreen() const { return (state_ == toplevelState::Fullscreen); };

    virtual bool isVirtual() const final { return 0; }
};

//childWindow
class ChildWindow : public Window
{
protected:
    ChildWindow();
    void create(window& parent, vec2i position, vec2ui size, windowContextSettings settings = windowContextSettings());

public:
    childWindow(window& parent, vec2i position, vec2ui size, windowContextSettings settings = windowContextSettings());

    const toplevelWindow* getTopLevelParent() const { return getParent()->getTopLevelParent(); };
    toplevelWindow* getTopLevelParent() { return getParent()->getTopLevelParent(); };

    virtual bool isVirtual() const final;
};
*/

}

