#pragma once

#include <ny/include.hpp>
#include <ny/eventHandler.hpp>
#include <ny/surface.hpp>
#include <ny/cursor.hpp>
#include <ny/data.hpp>
#include <ny/windowEvents.hpp>
#include <ny/windowDefs.hpp>

#include <nyutil/callback.hpp>
#include <nyutil/vec.hpp>

#include <memory>
#include <functional>

namespace ny
{

//todo: events masks for windows
//todo: implement all callback-add-functions for window correctly. can it be done with some template-like method?

//window class
class window : public eventHandler, public surface
{
protected:
    //position and max/min - size. size itself inherited from surface
    vec2i position_;
    vec2ui size_;

    vec2ui minSize_;
    vec2ui maxSize_;

    //states saved in window, not in the context
    bool focus_ : 1;
    bool mouseOver_ : 1;
    bool shown_ : 1;

    //current window cursor
    cursor cursor_;

    //windowContext. backend specific object. Core element of window. window itself is not able to change anything, it has to communicate with the backend through this object
    std::unique_ptr<windowContext> windowContext_;

    //stores window hints. most of window hints (e.g. Native<> or Maximize/Minimize etc. are not set by window class itself, but by the specific derivations of window
    //do NOT represent window states (as focus_, mouseOver_ etc) but general information about the window, which should not be changed by the window or its context after creation (of course they can be changed from the outside)
    unsigned long hints_ = 0;

    //files of the types listed in the dataType (basically vector of dataTypes) will generate a dataReceiveEvent when they are dropped on the window
    dataTypes dropAccept_ {};


    //callbacks
    callback<void(window&, drawContext&)> drawCallback_;
    callback<void(window&, const vec2ui&)> resizeCallback_;
    callback<void(window&, const vec2i&)> moveCallback_;
    callback<void(window&)> destroyCallback_;
    callback<void(window&, const focusEvent&)> focusCallback_;
    callback<void(window&, const showEvent&)> showCallback_;

    callback<void(window&, const mouseMoveEvent&)> mouseMoveCallback_;
    callback<void(window&, const mouseButtonEvent&)> mouseButtonCallback_;
    callback<void(window&, const mouseCrossEvent&)> mouseCrossCallback_;
    callback<void(window&, const mouseWheelEvent&)> mouseWheelCallback_;

    callback<void(window&, const keyEvent&)> keyCallback_;

    //events - have to be protected?
    virtual void mouseMove(std::unique_ptr<mouseMoveEvent>);
    virtual void mouseCross(std::unique_ptr<mouseCrossEvent>);
    virtual void mouseButton(std::unique_ptr<mouseButtonEvent>);
    virtual void mouseWheel(std::unique_ptr<mouseWheelEvent>);
    virtual void keyboardKey(std::unique_ptr<keyEvent>);
    virtual void windowSize(std::unique_ptr<sizeEvent>);
    virtual void windowPosition(std::unique_ptr<positionEvent>);
    virtual void windowDraw(std::unique_ptr<drawEvent>);
    virtual void windowShow(std::unique_ptr<showEvent>);
    virtual void windowFocus(std::unique_ptr<focusEvent>);

    //window is abstract class
    window();
    window(eventHandler& parent, vec2ui position, vec2ui size, const windowContextSettings& settings = windowContextSettings());
    void create(eventHandler& parent, vec2i position, vec2ui size, const windowContextSettings& settings = windowContextSettings());

    //windowContext functions are protected, derived classes can make public aliases if needed. User should not be able to change backend Hints for button e.g
    void setCursor(const cursor& curs);

    void setBackendHints(unsigned int backend, unsigned long hints);
    void addBackendHints(unsigned int backend, unsigned long hints);
    void removeBackendHints(unsigned int backend, unsigned long hints);

    void setWindowHints(unsigned long hints);
    void addWindowHints(unsigned long hints);
    void removeWindowHints(unsigned int hints);

    void setAcceptedDropTypes(const dataTypes& d);
    void addDropType(unsigned char type);
    void removeDropType(unsigned char type);

    virtual void draw(drawContext& dc);
    virtual bool checkValid() const;

public:
    virtual ~window();

    ////////////////////////////////////////////////////////////
    //virtual functions
    //todo: which of them have to be virtual? rethink - maybe some of the <basic window functions> should be virtual, too

    //returns if the given event was processed by the window
    virtual bool processEvent(std::unique_ptr<event> event) override;
    virtual void destroy() override;
    virtual bool valid() const override;

    virtual window* getParent() const override { return dynamic_cast<window*>(eventHandler::getParent()); }

    virtual toplevelWindow* getTopLevelParent() = 0;
    virtual const toplevelWindow* getTopLevelParent() const = 0;

    virtual bool isVirtual() const { return 0; }

    /////////////////////////////////////////////////////////////
    //basic window functions
    void setSize(vec2ui size);
    void setSize(unsigned int width, unsigned int height){ setSize(vec2ui(width, height)); };
    void setWidth(unsigned int width){ setSize(vec2ui(width, size_.y)); };
    void setHeight(unsigned int height){ setSize(vec2ui(size_.x, height)); };

    std::vector<childWindow*> getWindowChildren();
    window* getWindowAt(vec2i position);

    void setPosition(vec2i position);
    void setPosition(int x, int y){ setPosition(vec2i(x,y)); };
    void setXPosition(int x){ setPosition(vec2i(x, position_.y)); };
    void setYPosition(int y){ setPosition(vec2i(position_.x, y)); };

    void move(vec2i delta);
    void move(int dx, int dy){ move(vec2i(dx,dy)); };

    void setMinSize(vec2ui size);
    void setMinSize(unsigned int width, unsigned int height){ setMinSize(vec2ui(width, height)); };
    void setMinWidth(unsigned int width){ setMinSize(width, size_.y); };
    void setMinHeight(unsigned int height){ setMinSize(size_.x, height); };

    void setMaxSize(vec2ui size);
    void setMaxSize(unsigned int width, unsigned int height){ setMaxSize(vec2ui(width, height)); }
    void setMaxWidth(unsigned int width){ setMaxSize(vec2ui(width, size_.y)); };
    void setMaxHeight(unsigned int height){ setMaxSize(vec2ui(size_.x, height)); };

    void refresh();

    void show();
    void hide();
    void toggleShow();

    /////////////////////////////////////////////////////////////////////
    //callbacks
    template<typename F> std::unique_ptr<connection> onDraw(F&& func){ return drawCallback_.add(func); }
    template<typename F> std::unique_ptr<connection> onResize(F&& func){ return resizeCallback_.add(func); }
    template<typename F> std::unique_ptr<connection> onMove(F&& func){ return moveCallback_.add(func); }
    template<typename F> std::unique_ptr<connection> onDestroy(F&& func){ return destroyCallback_.add(func); }
    template<typename F> std::unique_ptr<connection> onFocus(F&& func){ return focusCallback_.add(func); }
    template<typename F> std::unique_ptr<connection> onShow(F&& func){ return showCallback_.add(func); } //sth like onShowState
    template<typename F> std::unique_ptr<connection> onMouseMove(F&& func){ return mouseMoveCallback_.add(func); }
    template<typename F> std::unique_ptr<connection> onMouseButton(F&& func){ return mouseButtonCallback_.add(func); }
    template<typename F> std::unique_ptr<connection> onMouseCross(F&& func){ return mouseCrossCallback_.add(func); }
    template<typename F> std::unique_ptr<connection> onMouseWheel(F&& func){ return mouseWheelCallback_.add(func); }
    template<typename F> std::unique_ptr<connection> onKey(F&& func){ return keyCallback_.add(func); }

    /////////////////////////////////////////////////////
    //const getters
    vec2i getPosition() const                           { return position_; }
    int getPositionX() const                            { return position_.x; }
    int getPositionY() const                            { return position_.y; }

    vec2ui getSize() const                              { return size_; }
    unsigned int getWidth() const                       { return size_.x; }
    unsigned int getHeight() const                      { return size_.y; }

    vec2ui getMinSize() const                           { return minSize_; }
    vec2ui getMinWidth() const                          { return minSize_.x; }
    vec2ui getMinHeight() const                         { return minSize_.y; }

    vec2ui getMaxSize() const                           { return maxSize_; }
    vec2ui getMaxWidth() const                          { return maxSize_.x; }
    vec2ui getMaxHeight() const                         { return maxSize_.y; }

    bool hasFocus() const                               { return focus_; }
    bool hasMouseOver() const                           { return mouseOver_; }
    bool isShown () const                               { return shown_; }

    unsigned long getWindowHints() const                { return hints_; }

    windowContext* getWindowContext() const             { return windowContext_.get(); }
    wc* getWC() const                                   { return windowContext_.get(); }

    const cursor& getCursor() const                     { return cursor_; }

    dataTypes getAcceptedDropTypes() const              { return dropAccept_; }
    bool dropTypeAccepted(unsigned char type) const     { return dropAccept_.contains(type); }

    //get data from windowContext, have to check if window is valid. defined in window.cpp
    windowContextSettings getWindowContextSettings() const;
    unsigned long getBackendHints() const;
};

//toplevel window
class toplevelWindow : public window
{
protected:
    //hints here not really needed
    //only for virtual handling, but they all are still saved in windowContext. rather make alias functions to access the on the WC stored hints
    unsigned char handlingHints_{};
    toplevelState state_{};
    std::string title_{};
    unsigned int borderSize_{};

    void mouseButton(std::unique_ptr<mouseButtonEvent> ev);
    void mouseMove(std::unique_ptr<mouseMoveEvent> ev);

    toplevelWindow();
    void create(vec2i position, vec2ui size, std::string title = " ", const windowContextSettings& settings = windowContextSettings());

public:
    toplevelWindow(vec2i position, vec2ui size, std::string title = " ", const windowContextSettings& settings = windowContextSettings());

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

    bool isCustomDecorated() const {  return (hints_ & windowHints::CustomDecorated); }
    bool isCustomMoved() const { return (hints_ & windowHints::CustomMoved); }
    bool isCustomResized() const { return (hints_ & windowHints::CustomResized); }

    //return if successful
    bool setCustomDecorated(bool set = 1);
    bool setCustomMoved(bool set = 1);
    bool setCustomResized(bool set = 1);

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
class childWindow : public window
{
protected:
    childWindow();
    void create(window& parent, vec2i position, vec2ui size, windowContextSettings settings = windowContextSettings());

public:
    childWindow(window& parent, vec2i position, vec2ui size, windowContextSettings settings = windowContextSettings());

    const toplevelWindow* getTopLevelParent() const { return getParent()->getTopLevelParent(); };
    toplevelWindow* getTopLevelParent() { return getParent()->getTopLevelParent(); };

    virtual bool isVirtual() const final;
};


}

