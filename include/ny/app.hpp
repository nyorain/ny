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

friend class backend; //calls registerBackend on init

protected:
    static app* mainApp;
    static std::vector<backend*> backends; //ALL built in . from these one available backend is chose in init()
    static void registerBackend(backend& e);

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

    static app* nyMainApp(){ return mainApp; };

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

    //todo
    //nyutil::msgThread eventDispatcher_ {};
    //std::deque<event*> events_ {};

    //replace from eventHandler
    virtual void create(eventHandler& parent) override {};

public:
    app();
    virtual ~app();

    //virtual core
    virtual bool init(const appSettings& settings = appSettings());

    virtual int mainLoop();
    virtual void exit(int exitReason = exitReason::unknown);
    virtual void onError();

    //eventHandler
    virtual void removeChild(eventHandler& handler) override;
    virtual void destroy() override;
    virtual void reparent(eventHandler& newParent) override {};

    //get/set
    window* getMouseOver() const { return mouseOver_; };
    window* getFocus() const { return focus_; };

    appContext* getAppContext() const { return appContext_.get(); };
    ac* getAC() const { return getAppContext(); };

    backend* getBackend() const { return backend_; }
    void setBackend(backend& b) { if(!valid_) backend_ = &b; }

    const appSettings& getSettings() const { return settings_; }
    const std::string& getName() const { return settings_.name; }

    bool isValid() const { return valid_; };
    bool mainThread() const { return std::this_thread::get_id() == mainThreadID_; };

    threadpool* getThreadPool() const { return threadpool_.get(); }

    eventLoop& getEventLoop() { return mainLoop_; }
    const eventLoop& getEventLoop() const { return mainLoop_; }

public:
    void sendEvent(event& ev, eventHandler& handler);
    void sendEvent(event& ev);

    void keyboardKey(keyEvent& event);
    void mouseMove(mouseMoveEvent& event);
    void mouseButton(mouseButtonEvent& event);
    void mouseCross(mouseCrossEvent& event);
    void windowFocus(focusEvent& event);
};


}
