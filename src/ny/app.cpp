#include <ny/app.hpp>

#include <ny/eventHandler.hpp>
#include <ny/event.hpp>
#include <ny/error.hpp>
#include <ny/appContext.hpp>
#include <ny/backend.hpp>
#include <ny/window.hpp>
#include <ny/font.hpp>
#include <ny/keyboard.hpp>
#include <ny/mouse.hpp>

#include <nyutil/thread.hpp>
#include <nyutil/time.hpp>
#include <nyutil/misc.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

namespace ny
{

app* app::mainApp = nullptr;
std::vector<backend*> app::backends;

//app////////////////////////////////////////////////
app::app() : eventHandler(), threadpool_{nullptr}
{
    if(nyMainApp() != nullptr)
    {
        nyWarning("there can be only one app");
        return;
    }

    mainApp = this;
}

app::~app()
{
    exit();

    if(nyMainApp() == this)
        mainApp = nullptr;
}

void app::registerBackend(backend& b)
{
    backends.push_back(&b);
}

bool app::init(const appSettings& settings)
{
    settings_ = settings;

    if(!backend_)
    {
        for(unsigned int i(0); i < backends.size(); i++)
        {
            if(settings_.allBackends || contains(settings_.allowedBackends, backends[i]->id))
            {
                if(backends[i]->isAvailable())
                {
                    backend_ = backends[i];
                    break;
                }
                else
                {
                    //not available
                }
            }
        }

        if(!backend_)
        {
            nyError("in app::init: No matching backend found.");
            return false;
        }
    }

    if(this != nyMainApp())
    {
        nyWarning("Only mainApp can be initialized. There can be only one app");
        return false;
    }

    try
    {
        appContext_ = backend_->createAppContext();
    }
    catch(std::exception& err)
    {
        nyError(err);
        appContext_.reset();
        valid_ = 0;
        return false;
    }

    font::getDefault().loadFromName(settings_.defaultFont);


    if(settings_.threadpoolSize > 0)
    {
        threadpool_.reset(new threadpool(settings_.threadpoolSize));
    }
    else if(settings_.threadpoolSize < 0)
    {
         threadpool_.reset(new threadpool()); //auto size
    }
    //if threadpoolSize in appSetitngs == 0, no threadpool is created

    valid_ = 1;
    return 1;
}

int app::mainLoop()
{
    if(!valid_) return 0;

    mainThreadID_ = std::this_thread::get_id();
    eventDispatcher_ = std::thread(&app::eventDispatcher, this);

    exitReason_ = exitReason::noEventSources;
    mainLoop_.run();

    //clean up
    exit_ = 1;

    threadpool_.reset();

    eventCV_.notify_one();
    eventDispatcher_.join();

    destroy(); //from eventHandler

    appContext_->exit();
    valid_ = 0;

    return exitReason_;
}

void app::exit(int reason)
{
    //todo
    exitReason_ = reason;
    mainLoop_.stop();

/*
    exit_ = 1;

    threadpool_.reset();

    eventCV_.notify_one();
    eventDispatcher_.join();

    destroy(); //from eventHandler

    appContext_->exit();
    valid_ = 0;
*/
}

bool app::valid() const
{
    return valid_;
}

bool app::removeChild(eventHandler& child)
{
    if(focus_ == &child) focus_ = nullptr;
    if(mouseOver_ == &child) mouseOver_ = nullptr;

    bool ret = eventHandler::removeChild(child);

    if(getChildren().size() == 0 && settings_.exitWithoutChildren)
    {
        exit();
    }

    return ret;
}

void app::destroy()
{
    eventHandler::destroy();
}

void app::sendEvent(std::unique_ptr<event> ev)
{
    //if(ev && ev->handler) ev->handler->processEvent(*ev);
    //return;

    //todo: check if old events can be overriden
    if(!ev.get())
    {
        nyWarning("app::sendEvent: invalid event");
        return;
    }

    { //critical begin
        std::lock_guard<std::mutex> lck(eventMtx_);

        if(ev->overrideable())
        {
            for(auto& stored : events_)
            {
                if(stored.get() && stored->type() == ev->type())
                {
                    stored = std::move(ev);
                    break;
                }
            }
        }

        if(ev.get())
        {
            if(ev->type() == eventType::windowSize)
                events_.emplace_front(std::move(ev));
            else
                events_.emplace_back(std::move(ev));
        }
    } //critical end

    eventCV_.notify_one();
}

void app::sendEvent(const event& ev)
{
    sendEvent(ev.clone());
}

void app::eventDispatcher()
{
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
void app::windowFocus(std::unique_ptr<focusEvent> event)
{
    if(!event->handler) return;

    if(event->focusGained)focus_ = dynamic_cast<window*>(event->handler);
    else if(event->handler == focus_)focus_ = nullptr;

    sendEvent(std::move(event));
}

void app::keyboardKey(std::unique_ptr<keyEvent> event)
{
    if(event->pressed) keyboard::keyPressed(event->key);
    else keyboard::keyReleased(event->key);

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
void app::mouseMove(std::unique_ptr<mouseMoveEvent> event)
{
    mouse::setPosition(event->position);
    if(!mouseOver_)
    {
        if(event->handler)
        {
            mouseOver_ = dynamic_cast<window*>(event->handler);
        }
    }
    else if(!event->handler)
    {
        event->handler = mouseOver_;
    }

    if(!event->handler)
    {
        nyWarning("app::mouseMove: unaware of current mouse-over window and received mouseMove event without valid handler");
        return;
    }

    if(mouseOver_)
    {
        window* child = mouseOver_->getTopLevelParent()->getWindowAt(event->position);

        if(child && child != mouseOver_)
        {
            mouseCross(std::make_unique<mouseCrossEvent>(mouseOver_, 0));
            mouseCross(std::make_unique<mouseCrossEvent>(child, 1));

            mouseOver_ = child;
        }

    }

    sendEvent(std::move(event));
}

void app::mouseButton(std::unique_ptr<mouseButtonEvent> event)
{
    if(event->pressed)mouse::buttonPressed(event->button);
    else mouse::buttonReleased(event->button);

    if(!mouseOver_ && !event->handler)
    {
        nyWarning("app::mouseButton: not able to send mouseButtonEvent, no valid handler");
        return;
    }
    if(mouseOver_)
    {
        event->handler = mouseOver_; //needed because of virtual windows
    }

    sendEvent(std::move(event));
}

void app::mouseCross(std::unique_ptr<mouseCrossEvent> event)
{
    if(event->handler == nullptr)
    {
        nyWarning("app::mouseCross: event with no handler");
        return;
    }

    //if childWindow is directly at the edge
    window* w = dynamic_cast<window*>(event->handler);
    if(w && event->entered)
    {
        window* child = w->getWindowAt(event->position);
        if(child) event->handler = child;
    }

    if(event->entered) mouseOver_ = w;
    else if(event->handler == mouseOver_) mouseOver_ = nullptr;

    sendEvent(std::move(event));
}

void app::mouseWheel(std::unique_ptr<mouseWheelEvent> event)
{
    mouse::wheelMoved(event->value);
    sendEvent(std::move(event));
}


//
/*
void app::addListenerFor(unsigned int id, eventHandler* ev)
{
    listeners_[id].push_back(ev);
}

void app::removeListenerFor(unsigned int id, eventHandler* ev)
{
    for(unsigned int i(0); i < listeners_[id].size(); i++)
    {
        if(listeners_[id][i] == ev)
        {
            listeners_[id].erase(listeners_[id].begin() + i);
        }
    }
}
*/

void app::onError()
{
    if(settings_.onError == errorAction::Exit)
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

//getMainApp
app* nyMainApp()
{
    return app::nyMainApp();
}


}
