#include <ny/app/app.hpp>

#include <ny/app/event.hpp>
#include <ny/backend/appContext.hpp>
#include <ny/backend/backend.hpp>
#include <ny/app/keyboard.hpp>
#include <ny/app/mouse.hpp>
#include <ny/window/window.hpp>
#include <ny/draw/freeType.hpp>

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

	//TODO: hack to create (static) freetype lib before any (static) freetype font
	FreeTypeLibrary::instance();

    appContext_ = backend_->createAppContext();
}

App::~App()
{
    this->exit();
	EventDispatcher::exit();
}

int App::mainLoop()
{
	int ret = appContext_->mainLoop();
	return ret;
}

void App::exit()
{
	if(appContext_) appContext_->exit();
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
		exit();
	}
}

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
