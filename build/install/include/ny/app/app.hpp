#pragma once

#include "include.hpp"
#include "app/eventHandler.hpp"

#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>

namespace ny
{

void defaultErrorHandler(const error& err);
app* getMainApp();

enum class errorAction
{
    Exit,
    Continue,
    AskWindow, //with fallback to AskConsole
    AskConsole
};

class appSettings
{
public:
    std::string name;
    bool threadSafe = 0;
    bool exitWithoutChildren = 1;
    errorAction onError = errorAction::AskWindow;
    std::vector<unsigned int> allowedBackends;
    bool allBackends = 1;
    size_t threadpoolSize = 4;
    bool backendThread = 0;

    int argc = 0;
    const char** argv = nullptr;

    void(*errorHandler)(const error&) = &defaultErrorHandler;
};

class app : public eventHandler
{
protected:
    static app* mainApp;
    static std::vector<backend*> backends; //ALL built in backends. from these one available backend is chose in init()

    eventHandler* focus_; //eventHandler which has current focus
    window* mouseOver_; //eventHandler on which is the mouse

    appContext* appContext_;

    bool existing_ : 1; //if app wasnt exited
    bool valid_ : 1; //if the app was initialized (correctly)
    bool mainLoop_ : 1; //if the app is in mainLoop

    backend* backend_; //the chosen backend. only existing if(valid_)
    threadpool* threadpool_;
    std::thread::id mainThreadID_; //holds the thread id that is executing the mainLoop. only valid if(mainLoop_)
    appSettings settings_;

    std::thread* backendThread_;

    std::condition_variable eventCV_;
    std::mutex eventMtx_;
    std::queue<std::pair<eventHandler*, event*>> events_;

    //replace from eventHandler
    virtual void create(eventHandler& parent){};

public:
    static app* getMainApp(){ return mainApp; };

    app();
    virtual ~app();

    virtual bool init();
    virtual bool init(unsigned int argCount, const char** args);
    virtual bool init(appSettings settings);

    virtual int mainLoop();

    virtual void exitApp();

    virtual bool optionRegistered(std::string option, std::string arg = "");

    //functions inherited from eventHandler
    virtual void removeChild(eventHandler* handler);
    virtual void destroy();
    virtual void reparent(eventHandler& newParent){};

    //eventFunctions
    virtual void sendEvent(event& event, eventHandler& handler);

    virtual void keyboardKey(keyEvent& event);

    virtual void mouseMove(mouseMoveEvent& event);
    virtual void mouseButton(mouseButtonEvent& event);
    virtual void mouseCross(mouseCrossEvent& event);
    virtual void mouseWheel(mouseWheelEvent& event);

    virtual void windowSize(sizeEvent& event);
    virtual void windowPosition(positionEvent& event);
    virtual void windowDraw(drawEvent& event);
    virtual void windowFocus(focusEvent& event);

    virtual void destroyHandler(destroyEvent& event);

    virtual window* getMouseOver() const { return mouseOver_; };
    virtual eventHandler* getFocus() const { return focus_; };

    virtual appContext* getAppContext() const { return appContext_; };
    ac* getAC() const { return getAppContext(); };

    virtual backend* getBackend() const { return backend_; };
    void setBackend(backend* b) { backend_ = b; }

    //virtual void addListenerFor(unsigned int, eventHandler*);
    //virtual void removeListenerFor(unsigned int, eventHandler*);

    virtual void errorRun(const error& e);

    virtual void addTask(taskBase* b);

    virtual bool isThreadSafe() const { return settings_.threadSafe; };
    std::string getName() const { return settings_.name; };
    errorAction getErrorAction() const { return settings_.onError; };
    std::vector<unsigned int> getAllowedBackends() const { return settings_.allowedBackends; };

    bool isValid() const { return valid_; };

    bool isMainThread() const { return std::this_thread::get_id() == mainThreadID_; };
};


}
