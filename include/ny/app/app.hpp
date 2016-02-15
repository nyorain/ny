#pragma once

#include <ny/include.hpp>
#include <ny/base/loopControl.hpp>
#include <nytl/nonCopyable.hpp>

#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <atomic>

namespace ny
{

///\brief Main Application class.
///\details The main responsibilities on ny::App are to initialize an ny::Backend with an
///ny::AppContext as well as dispatching all received events (ny::EventDispatcher).
class App : public NonMoveable
{
public:
	enum class ErrorAction
	{
		throwing,
    	continuing,
    	askWindow, //with fallback to AskConsole
    	askConsole
	};

	struct Settings
	{
		Settings(){};

		std::string name {"nyApp"};
		bool exitWithoutWindows = 1;
		ErrorAction errorAction = ErrorAction::askWindow;
		std::vector<std::string> allowedBackends;
		bool allBackends = 1;
		bool multiThreaded = 1;
	};

protected:
    Settings settings_;
    Backend* backend_ {nullptr}; 
    std::unique_ptr<AppContext> appContext_;
	LoopControl* mainLoopControl_ {nullptr};

	std::thread backendThread_;
	LoopControl backendLoopControl_;

	std::size_t windowCount_ = 0; //to make exiting possible when last window closes
	std::unique_ptr<EventDispatcher> eventDispatcher_ {nullptr};

protected:
	friend class Window;
	void windowCreated();
	void windowClosed();

public:
    App(const Settings& settings = {});
    virtual ~App();

    virtual int run(LoopControl& control);
	virtual int run();

    virtual void error(const std::string& msg);

    AppContext& appContext() const { return *appContext_.get(); };
    Backend& backend() const { return *backend_; }

	std::size_t windowCount() const { return windowCount_; }

    const Settings& settings() const { return settings_; }
    const std::string& name() const { return settings_.name; }
};

}
