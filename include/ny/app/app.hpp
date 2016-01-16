#pragma once

#include <ny/app/include.hpp>
#include <ny/app/eventHandler.hpp>

#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <queue>
#include <condition_variable>

namespace ny
{

App* nyMainApp();

class App : public EventHandlerRoot
{
public:
	enum class ErrorAction
	{
		Exit,
    	Continue,
    	AskWindow, //with fallback to AskConsole
    	AskConsole
	};

	struct Settings
	{
		std::string name;
		bool exitWithoutChildren = 1;
		App::ErrorAction onError = ErrorAction::AskWindow;
		std::vector<std::string> allowedBackends;
		bool allBackends = 1;
		int threadpoolSize = -1; //auto size, 0 for no threadpool
		bool useEventThread = 1;
	};

public:
	static App* app();

protected:
	static App* appFunc(App* app = nullptr, bool reg = 0);	

protected:
    Settings settings_;

    std::thread::id loopThreadID_;
    std::thread::id eventThreadID_;

    Backend* backend_ {nullptr}; //the chosen backend. only existing if(valid_), one of backends

    std::unique_ptr<AppContext> appContext_;

    //changed/read by eventLoop and by eventDispatcher thread:
    std::atomic<Window*> focus_ {nullptr}; //eventHandler which has current focus
    std::atomic<Window*> mouseOver_ {nullptr}; //eventHandler on which is the mouse

    std::atomic<bool> exit_ {0};
    std::atomic<bool> mainLoop_ {0};

    //event dispatching
    std::thread eventDispatcher_;
    std::deque<std::unique_ptr<Event>> events_;
    std::mutex eventMtx_;
    std::condition_variable eventCV_;

    //dispatcher func
    void eventDispatcher();

    //eventHandler
    virtual bool removeChild(EventHandlerNode& handler) override;
    virtual void destroy() override;

public:
    App(const Settings& settings = Settings{});
    virtual ~App();

	//
    virtual int mainLoop();
    virtual void exit();
    virtual void onError();

    //get/set
    Window* mouseOver() const { return mouseOver_; };
    Window* focus() const { return focus_; };

    AppContext& appContext() const { return *appContext_.get(); };
    Backend& backend() const { return *backend_; }

    const Settings& settings() const { return settings_; }
    const std::string& name() const { return settings_.name; }

    bool loopThread() const { return std::this_thread::get_id() == loopThreadID_; };
    bool eventThread() const { return std::this_thread::get_id() == eventThreadID_; };

public:
    void sendEvent(std::unique_ptr<Event>&& ev);
    void sendEvent(const Event& ev);

    void keyboardKey(std::unique_ptr<KeyEvent>&& event);
    void mouseMove(std::unique_ptr<MouseMoveEvent>&& event);
    void mouseButton(std::unique_ptr<MouseButtonEvent>&& event);
    void mouseCross(std::unique_ptr<MouseCrossEvent>&& event);
    void mouseWheel(std::unique_ptr<MouseWheelEvent>&& event);
    void windowFocus(std::unique_ptr<FocusEvent>&& event);
};


}
