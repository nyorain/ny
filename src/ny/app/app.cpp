#include <ny/app/app.hpp>

#include <ny/app/event.hpp>
#include <ny/backend/appContext.hpp>
#include <ny/backend/backend.hpp>
#include <ny/window/window.hpp>
#include <ny/window/windowEvents.hpp>
#include <ny/app/keyboard.hpp>
#include <ny/app/mouse.hpp>

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
    loopThreadID_ = std::this_thread::get_id();
    eventDispatcher_ = std::thread(&App::eventDispatcher, this);

    mainLoop_ = 1;
	exit_ = 0;

	int ret = appContext_->mainLoop();

    //clean up
    mainLoop_ = 0;
    exit_ = 1;

    eventCV_.notify_one();
    eventDispatcher_.join();

    destroy(); //from eventHandler

	return ret;
}

void App::exit()
{
	exit_.store(1);
}

bool App::removeChild(EventHandlerNode& child)
{
    if(focus_.load() == &child) focus_ = nullptr;
    if(mouseOver_.load() == &child) mouseOver_ = nullptr;

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

void App::sendEvent(EventPtr&& event)
{
    if(!settings_.useEventThread)
    {
        if(!mainLoop_)
            return;

        if(event->handler) event->handler->processEvent(*event);
        else nytl::sendWarning("app::sendEvent: received event without valid event handler");

        return;
    }

    //if(ev && ev->handler) ev->handler->processEvent(*ev);
    //return;

    //todo: check if old events can be overriden
    if(!event.get())
    {
		nytl::sendWarning("app::sendEvent: invalid event");
        return;
    }

    { //critical begin
        std::lock_guard<std::mutex> lck(eventMtx_);

        if(event->overrideable())
        {
            for(auto& stored : events_)
            {
                if(stored.get() && stored->type() == event->type())
                {
                    stored = std::move(event);
                    break;
                }
            }
        }

        if(event.get())
        {
            events_.emplace_back(std::move(event));
        }
    } //critical end

    eventCV_.notify_one();
}

void App::sendEvent(const Event& ev)
{
    sendEvent(clone(ev));
}

void App::eventDispatcher()
{
    eventThreadID_ = std::this_thread::get_id();
    std::unique_lock<std::mutex> lck(eventMtx_);

    while(!exit_.load())
    {
        while(events_.empty() && !exit_.load()) eventCV_.wait(lck);
        if(exit_.load()) return;

        auto ev = std::move(events_.front());
        events_.pop_front();

        lck.unlock();
        if(ev->handler) ev->handler->processEvent(*ev);
        lck.lock();
    }
}

//keyboard Events
void App::windowFocus(std::unique_ptr<FocusEvent>&& event)
{
    if(!event->handler) return;

    if(event->focusGained) focus_ = dynamic_cast<Window*>(event->handler);
    else if(event->handler == focus_) focus_ = nullptr;

    sendEvent(std::move(event));
}

void App::keyboardKey(std::unique_ptr<KeyEvent>&& event)
{
    Keyboard::keyPressed(event->key, event->pressed);

    if(!event->handler)
    {
        if(!focus_)
            return;

        event->handler = focus_;
    }
    else if(event->handler != focus_)
    {
        //strange, what do to here?
    }

    sendEvent(std::move(event));
}


//mouseEvents
void App::mouseMove(std::unique_ptr<MouseMoveEvent>&& event)
{
    Mouse::position(event->position);
    sendEvent(std::move(event));
}

void App::mouseButton(std::unique_ptr<MouseButtonEvent>&& event)
{
    if(event->pressed) Mouse::buttonPressed(event->button, 1);
    else Mouse::buttonPressed(event->button, 0);

    if(!mouseOver_ && !event->handler)
    {
		nytl::sendWarning("App::MouseButton: event with invalid handler, no mouseOver");
        return;
    }
    if(mouseOver_)
    {
        event->handler = mouseOver_; //needed because of virtual windows
    }

    sendEvent(std::move(event));
}

void App::mouseCross(std::unique_ptr<MouseCrossEvent>&& event)
{
    if(event->handler == nullptr)
    {
		nytl::sendWarning("App::mouseCross: event with invalid handler");
        return;
    }

    sendEvent(std::move(event));
}

void App::mouseWheel(std::unique_ptr<MouseWheelEvent>&& event)
{
    Mouse::wheelMoved(event->value);
    sendEvent(std::move(event));
}

void App::onError()
{
    if(settings_.onError == ErrorAction::Exit)
    {
        std::exit(-1);
    }

    std::cout << "Error occured. Continue? (y/n)" << std::endl;

    std::string s;
    std::cin >> s;

    if(s == "1" || s == "y" || s == "yes")
    {
        return;
    }

    else
    {
        std::exit(-1);
        //throw e;
    }
}

}
