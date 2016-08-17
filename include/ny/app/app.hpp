#pragma once

#include <ny/include.hpp>
#include <ny/base/loopControl.hpp>
#include <ny/base/eventDispatcher.hpp>
#include <nytl/nonCopyable.hpp>

#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <atomic>

namespace ny
{

//TODO
///\brief Main Application class.
class App : public NonMovable
{
public:
	enum class ErrorAction
	{
		throwing,
    	continuing,
    	askWindow, //with fallback to askConsole
    	askConsole
	};

	struct Settings
	{
		Settings(){};

		std::string name {"nyApp"};
		bool exitWithoutWindows = 1;
		ErrorAction errorAction = ErrorAction::askWindow;
		std::vector<std::string> allowedBackends;
		bool allBackends = true;
		bool multithreaded = true;
	};

public:
    App(const Settings& settings = {});
    virtual ~App();

    virtual int run(LoopControl& control);
	virtual int run();

	virtual bool dispatch();
	EventDispatcher& dispatcher() { return *eventDispatcher_; }

    virtual void error(const std::string& msg);

	//getters
    AppContext& appContext() const { return *appContext_.get(); };
    Backend& backend() const { return *backend_; }

	std::size_t windowCount() const { return windowCount_; }
    const Settings& settings() const { return settings_; }
    const std::string& name() const { return settings_.name; }

	const Mouse& mouse() const { return *mouse_; }
	const Keyboard& keyboard() const { return *keyboard_; }

	Mouse& mouse() { return *mouse_; }
	Keyboard& keyboard() { return *keyboard_; }

protected:
    Settings settings_;
    Backend* backend_ {nullptr};
    std::unique_ptr<AppContext> appContext_;
	LoopControl* mainLoopControl_ {nullptr};

	std::atomic<bool> exit_ {0};
	std::thread dispatcherThread_;
	LoopControl dispatcherLoopControl_;

	std::size_t windowCount_ = 0; //to make exiting possible when last window closes
	std::unique_ptr<EventDispatcher> eventDispatcher_ {nullptr};

	std::unique_ptr<Mouse> mouse_;
	std::unique_ptr<Keyboard> keyboard_;

protected:
	friend class Window;
	void windowCreated();
	void windowClosed();
};

}
