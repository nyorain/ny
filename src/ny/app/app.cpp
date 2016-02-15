#include <ny/app/app.hpp>

#include <ny/base/event.hpp>
#include <ny/base/loopControl.hpp>
#include <ny/backend/appContext.hpp>
#include <ny/backend/backend.hpp>
#include <ny/app/keyboard.hpp>
#include <ny/app/mouse.hpp>
#include <ny/app/eventDispatcher.hpp>
#include <ny/window/window.hpp>

#include <nytl/misc.hpp>
#include <ny/base/log.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

namespace ny
{

//
App::App(const App::Settings& settings) : settings_(settings)
{
    if(!backend_)
    {
		auto backends = Backend::backends();
		auto available = std::vector<Backend*>{};

		for(auto* backend : backends)
			if(backend->available()) available.push_back(backend);


		for(auto& allowd : settings_.allowedBackends)
		{
			for(auto* backend : available)
			{
				if(allowd == backend->name())
				{
					backend_ = backend;
					break;
				}
			}
		}

		if(!backend_)
		{
			if(!settings_.allBackends || available.empty())
			{
				throw std::runtime_error("App::App: Could not query a backend");
			}

			//fallback, just choose first available
			backend_ = available[0];
		}
	}

    appContext_ = backend_->createAppContext();

	if(settings.multiThreaded)
	{
		eventDispatcher_.reset(new ThreadedEventDispatcher());
	}
	else
	{
		eventDispatcher_.reset(new EventDispatcher());
	}
}

App::~App()
{
	if(mainLoopControl_) mainLoopControl_->stop();
}

int App::run(LoopControl& control)
{
	bool mainLoop_ = 0;
	if(!mainLoopControl_)
	{
		mainLoopControl_ = &control;
		mainLoop_ = 1;
	}

	if(settings_.multiThreaded)
	{
		if(!backendThread_.joinable())
		{
			backendThread_ = std::thread(&AppContext::dispatchLoop, appContext_.get(), 
					backendLoopControl_);
		}

		auto& dispatcher = static_cast<ThreadedEventDispatcher&>(*eventDispatcher_);
		dispatcher.dispatchLoop(control);

		backendLoopControl_.stop();
	}

	else
	{
		appContext_->dispatchLoop(*eventDispatcher_, control);
	}

	if(mainLoop_)
	{
		mainLoopControl_ = nullptr;
	}

	return EXIT_SUCCESS;
}

int App::run()
{
	LoopControl control;
	return run(control);
}

void App::windowCreated()
{
	windowCount_++;
}

void App::windowClosed()
{
	windowCount_--;
	if(windowCount_ < 1 && settings_.exitWithoutWindows)
	{
		if(mainLoopControl_) mainLoopControl_->stop();
	}
}

void App::error(const std::string& msg)
{
    if(settings_.errorAction == ErrorAction::throwing)
    {
		throw std::runtime_error(msg);
    }
	else if (settings_.errorAction == ErrorAction::continuing)
	{
		return;
	}

	if(appContext_) //try window
	{
		/*
		MessageBox::Button result = MessageBox::Button::none;
		try
		{
			MessageBox message(*this, "Error Occured: " + msg, 
					MessageBox::Button::ok | MessageBox::Button::cancel);

			result = message.runModal();
		}
		catch(...)
		{
		}
		
		if(result != MessageBox::Button::none)
		{
			if(result == MessageBox::Button::ok) return;
			else throw std::runtime_error(msg);
		}
		*/
	}

    std::cout << "Error: " << msg << "\nContinue? (y/n)" << std::endl;

    std::string s;
    std::cin >> s;

    if(s == "1" || s == "y" || s == "yes")
    {
        return;
    }
    else
    {
		throw std::runtime_error(msg);
    }
}

}
