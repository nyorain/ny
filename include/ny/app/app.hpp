#pragma once

#include <ny/include.hpp>
#include <ny/app/eventHandler.hpp>
#include <ny/app/eventDispatcher.hpp>

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

class App : public EventDispatcher
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
		Settings(){};

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
    Backend* backend_ {nullptr}; //the chosen backend. only existing if(valid_), one of backends
    std::unique_ptr<AppContext> appContext_;

    //changed/read by eventLoop and by eventDispatcher thread:
    std::atomic<Window*> focus_ {nullptr}; //eventHandler which has current focus
    std::atomic<Window*> mouseOver_ {nullptr}; //eventHandler on which is the mouse

protected:
    void keyboardKey(Event& event);
    void mouseMove(Event& event);
    void mouseButton(Event& event);
    void mouseCross(Event& event);
    void mouseWheel(Event& event);
    void windowFocus(Event& event);

public:
    App(const Settings& settings = {});
    virtual ~App();

    virtual int mainLoop();
	virtual void exit();
    virtual void error(const std::string& msg);

	bool running() const { return !exit_; }

    Window* mouseOver() const { return mouseOver_; };
    Window* focus() const { return focus_; };

    AppContext& appContext() const { return *appContext_.get(); };
    Backend& backend() const { return *backend_; }

    const Settings& settings() const { return settings_; }
    const std::string& name() const { return settings_.name; }
};

}
