#include <ny/app/app.hpp>

#include <ny/app/event.hpp>
#include <ny/backend/appContext.hpp>
#include <ny/backend/backend.hpp>
#include <ny/app/keyboard.hpp>
#include <ny/app/mouse.hpp>
#include <ny/window/window.hpp>

#include <nytl/time.hpp>
#include <nytl/misc.hpp>
#include <nytl/log.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

namespace ny
{

//
App* nyMainApp()
{
	return App::app();
}

App* App::app()
{
	return appFunc();
}

App* App::appFunc(App* app, bool reg)
{
	static App* instance_ = nullptr;
	if(reg) instance_ = app;
	return instance_;
}

//
App::App(const App::Settings& settings) : settings_(settings)
{
	if(app())
	{
		throw std::logic_error("App::App: There can only be one app object");
	}

	appFunc(this, 1);

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

			//fallback, just choose forst available
			backend_ = available[0];
		}
	}

    appContext_ = backend_->createAppContext(*this);
}

App::~App()
{
    exit();
	appFunc(nullptr, 1);
}

int App::mainLoop()
{
	exit_ = 0;

	int ret = appContext_->mainLoop();
	return ret;
}

void App::exit()
{
	EventDispatcher::exit();
}

/*
bool App::removeChild(EventHandlerNode& child)
{
    if(focus_.load() == static_cast<Window*>(&child)) focus_ = nullptr;
    if(mouseOver_.load() == static_cast<Window*>(&child)) mouseOver_ = nullptr;

    bool ret = EventHandlerRoot::removeChild(child);
    if(children().size() == 0 && settings_.exitWithoutChildren)
    {
		this->exit();
    }

    return ret;
}

void App::destroy()
{
    EventHandlerRoot::destroy();
}
*/

//keyboard Events
void App::windowFocus(Event&)
{
}

void App::keyboardKey(Event&)
{
}


//mouseEvents
void App::mouseMove(Event&)
{
}

void App::mouseButton(Event&)
{
}

void App::mouseCross(Event&)
{
}

void App::mouseWheel(Event& event)
{
	MouseWheelEvent& ev = static_cast<MouseWheelEvent&>(event);
    Mouse::wheelMoved(ev.value);
}

void App::error(const std::string& msg)
{
    if(settings_.onError == ErrorAction::Exit)
    {
        std::exit(-1);
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
        std::exit(-1);
    }
}

}
