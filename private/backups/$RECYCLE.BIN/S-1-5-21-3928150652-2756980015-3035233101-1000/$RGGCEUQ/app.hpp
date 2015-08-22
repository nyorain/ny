#pragma once

#include <ny/include.hpp>
#include <ny/eventHandler.hpp>
#include <nyutil/eventLoop.hpp>

#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>

namespace ny
{

app* nyMainApp();

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
    std::string defaultFont {"sans-serif"};

    bool exitWithoutChildren = 1;
    errorAction onError = errorAction::AskWindow;

    std::vector<unsigned int> allowedBackends;
    bool allBackends = 1;

    int threadpoolSize = -1; //auto size, 0 for no threadpool
};

class app : public eventHandler
{

friend backend; //calls registerBackend on init

public:
    enum exitReason : int
    {
        failure = EXIT_FAILURE,
        success = EXIT_SUCCESS,

        unknown = 1,
        noChildren = 2,
        noEventSources = 3,
        userInput = 4,
        signal = 10 // + signal number
    };

protected:
    static app* mainApp;
    static std::vector<backend*> backends; //ALL built in . from these one available backend is chose in init()
    static void registerBackend(backend& e);

protected:
    window* focus_{nullptr}; //eventHandler which has current focus
    window* mouseOver_{nullptr}; //eventHandler on which is the mouse

    std::unique_ptr<appContext> appContext_;

    bool existing_{0}; //todo: exit
    bool valid_{0}; //if the app was initialized (correctly)

    int exitReason_{exitReason::unknown};

    backend* backend_ {nullptr}; //the chosen backend. only existing if(valid_), one of backends
    std::unique_ptr<threadpool> threadpool_;
    std::thread::id mainThreadID_; //holds the thread id that is executing the mainLoop. only valid if(mainLoop_)
    appSettings settings_;

    eventLoop mainLoop_;

    //replace from eventHandler
    virtual void create(eventHandler& parent) override {};

public:
    static app* nyMainApp(){ return mainApp; };

    app();
    virtual ~app();

    virtual bool init(const appSettings& settings = appSettings());

    virtual int mainLoop();
    virtual void exit(int exitReason = exitReason::unknown);

    //functions inherited from eventHandler
    virtual void removeChild(eventHandler& handler) override;
    virtual void destroy() override;
    virtual void reparent(eventHandler& newParent) override {};

    //loop
    eventLoop& getEventLoop() { return mainLoop_; }

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

    window* getMouseOver() const { return mouseOver_; };
    window* getFocus() const { return focus_; };

    appContext* getAppContext() const { return appContext_.get(); };
    ac* getAC() const { return getAppContext(); };

    backend* getBackend() const { return backend_; }
    void setBackend(backend& b) { if(!valid_) backend_ = &b; }

    virtual void error();

    const appSettings& getSettings() const { return settings_; }

    bool isValid() const { return valid_; };
    bool mainThread() const { return std::this_thread::get_id() == mainThreadID_; };
};


}
