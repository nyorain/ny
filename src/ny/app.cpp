#include <ny/app.hpp>

#include <ny/eventHandler.hpp>
#include <ny/event.hpp>
#include <ny/error.hpp>
#include <ny/appContext.hpp>
#include <ny/backend.hpp>
#include <ny/window.hpp>
#include <ny/font.hpp>

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
    if(!valid_)
        return 0;

    mainThreadID_ = std::this_thread::get_id();

    exitReason_ = exitReason::noEventSources;
    mainLoop_.run();

    return exitReason_;

    /*
    if(!valid_ || mainLoop_)
        return 0;

    mainThreadID_ = std::this_thread::get_id();

    mainLoop_ = 1;

    while(existing_ && valid_)
    {
        if(!appContext_->mainLoop()) //should be blocking
        {
            valid_ = 0;
        }
    }

    mainLoop_ = 0;
    exitApp();

    return 1;
    */
}

void app::exit(int reason)
{
    //todo
    exitReason_ = reason;

    mainLoop_.stop();

    existing_ = 0;
    valid_ = 0;

    threadpool_.reset();

    destroy(); //from eventHandler

    appContext_->exit();
}


void app::removeChild(eventHandler& child)
{
    if(focus_ == &child) focus_ = nullptr;
    if(mouseOver_ == &child) mouseOver_ = nullptr;

    eventHandler::removeChild(child);

    if(children_.size() == 0 && settings_.exitWithoutChildren)
    {
        exit();
    }
}

void app::destroy()
{
    eventHandler::destroy();
}

void app::sendEvent(event& ev, eventHandler& handler)
{
    handler.lock();
    handler.processEvent(ev);
    handler.unlock();
}

void app::sendEvent(event& ev)
{
    if(ev.handler)
        sendEvent(ev, *ev.handler);
}

//keyboard Events
void app::windowFocus(focusEvent& event)
{
    if(event.handler == nullptr) return;

    if(event.state == focusState::gained)focus_ = dynamic_cast<window*>(event.handler);
    else if(event.state == focusState::lost && event.handler == focus_)focus_ = nullptr;

    sendEvent(event, *event.handler);
}

void app::keyboardKey(keyEvent& event)
{
    if(event.state == pressState::pressed)keyboard::keyPressed(event.key);
    else keyboard::keyReleased(event.key);

    if(focus_) sendEvent(event, *focus_);
}


//mouseEvents
void app::mouseMove(mouseMoveEvent& event)
{
    mouse::setPosition(event.position);

    if(!mouseOver_)
    {
        if(event.handler)
        {
            mouseOver_ = dynamic_cast<window*>(event.handler);
        }
        else
        {
            return;
        }
    }

    if(mouseOver_)
    {
        window* child = mouseOver_->getTopLevelParent()->getWindowAt(event.position);

        if(child && child != mouseOver_)
        {
            mouseCrossEvent leaveEv;
            leaveEv.handler = mouseOver_;
            leaveEv.state = crossType::left;
            mouseCross(leaveEv);

            mouseCrossEvent enterEv;
            enterEv.handler = child;
            enterEv.state = crossType::entered;
            mouseCross(enterEv);

            mouseOver_ = child;
        }
    }

    sendEvent(event, *mouseOver_);
}

void app::mouseButton(mouseButtonEvent& event)
{
    if(event.state == pressState::pressed)mouse::buttonPressed(event.button);
    else mouse::buttonReleased(event.button);

    if(mouseOver_) sendEvent(event, *mouseOver_);
}

void app::mouseCross(mouseCrossEvent& event)
{
    if(event.handler == nullptr) return;

    //if childWindow is directly at the edge
    window* w = dynamic_cast<window*>(event.handler);
    if(w && event.state == crossType::entered)
    {
        window* child = w->getWindowAt(event.position);
        if(child) event.handler = child;
    }

    if(event.state == crossType::entered) mouseOver_ = w;
    else if(event.state == crossType::left && event.handler == mouseOver_) mouseOver_ = nullptr;

    sendEvent(event, *event.handler);
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
