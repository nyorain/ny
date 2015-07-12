#include <ny/app.hpp>

#include <ny/eventHandler.hpp>
#include <ny/event.hpp>
#include <ny/error.hpp>
#include <ny/appContext.hpp>
#include <ny/backend.hpp>
#include <ny/window.hpp>

#include <ny/util/thread.hpp>
#include <ny/util/time.hpp>
#include <ny/util/misc.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

namespace ny
{

app* app::mainApp = nullptr;
std::vector<backend*> app::backends;

//app/////////////////////////////////////////7
app::app() : eventHandler(), focus_(nullptr), mouseOver_(nullptr), appContext_(nullptr), existing_(1), valid_(0), mainLoop_(0), backend_(nullptr), threadpool_(nullptr), settings_()
{
    if(getMainApp() != nullptr)
    {
        sendWarning("there can be only one app");
        return;
    }

    mainApp = this;
}

app::~app()
{
    exitApp();

    mainApp = nullptr;
}

void app::registerBackend(backend& b)
{
    backends.push_back(&b);
}

bool app::init()
{
    return init(appSettings());
}

bool app::init(unsigned int argCount, const char** args)
{
    appSettings a;

    a.argc = argCount;
    a.argv = args;

    return init(a);
}

bool app::init(appSettings settings)
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
            sendError("in app::init: No matching backend found.");
            return false;
        }
    }

    if(this != getMainApp())
    {
        sendWarning("Only getMainApp() can be initialized. There can be only one app");
        return false;
    }


    for(unsigned int i(0); i < (unsigned int) settings_.argc; i++)
    {
        std::vector<std::string> vec = split(settings_.argv[i], '=');
        if(vec.size() > 1) optionRegistered(vec[0], vec[1]);
        else if(vec.size() > 0) optionRegistered(vec[0]);
    }

    try
    {
       appContext_ = backend_->createAppContext();
    }

    catch(std::exception& err)
    {
        sendError(err);

        delete appContext_;
        appContext_ = nullptr;
        valid_ = 0;

        return false;
    }

    threadpool_ = new threadpool(settings_.threadpoolSize);

    valid_ = 1;
    return 1;
}

int app::mainLoop()
{
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
}


void app::addTask(taskBase* b)
{
    if(!valid_)
        return;

    threadpool_->addTask(b);
}


void app::exitApp()
{
    existing_ = 0;
    valid_ = 0;

    if(threadpool_)
    {
        delete threadpool_;
        threadpool_ = nullptr;
    }

    destroy(); //from eventHandler

    if(appContext_)
    {
        delete appContext_;
        appContext_ = nullptr;
    }
}

bool app::optionRegistered(std::string option, std::string arg)
{
    if(option == "--nytest" || option == "-nt")
    {
        std::cout << "LoggingTest from Commandline option t" << std::endl;
        return 1;
    }

    return 0;
}


void app::removeChild(eventHandler& child)
{
    if(focus_ == &child) focus_ = nullptr;
    if(mouseOver_ == &child) mouseOver_ = nullptr;

    eventHandler::removeChild(child);

    if(children_.size() == 0 && settings_.exitWithoutChildren)
    {
        exitApp();
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


//keyboard Events
void app::windowFocus(focusEvent& event)
{
    if(event.handler == nullptr)return;

    if(event.state == focusState::gained)focus_ = event.handler;
    else if(event.state == focusState::lost && event.handler == focus_)focus_ = nullptr;

    sendEvent(event, *event.handler);
}

void app::keyboardKey(keyEvent& event)
{
    if(event.state == pressState::pressed)keyboard::pressKey(event.key);
    else keyboard::releaseKey(event.key);

    sendEvent(event, *focus_);
}


//mouseEvents
void app::mouseMove(mouseMoveEvent& event)
{
    mouse::setPosition(event.position);

    if(!mouseOver_)
    {
        if(event.handler)
        {
            mouseOver_ = event.handler;
        }
        else
        {
            return;
        }
    }

    if(mouseOver_ != nullptr)
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
    if(event.state == pressState::pressed)mouse::pressButton(event.button);
    else mouse::releaseButton(event.button);

    sendEvent(event, *mouseOver_);
}

void app::mouseCross(mouseCrossEvent& event)
{
    if(event.handler == nullptr) return;

    //if childWindow is directly at the edge
    window* w = event.handler;
    if(w && event.state == crossType::entered)
    {
        window* child = w->getWindowAt(event.position);
        if(child) event.handler = child;
    }

    if(event.state == crossType::entered) mouseOver_ = event.handler;
    else if(event.state == crossType::left && event.handler == mouseOver_) mouseOver_ = nullptr;

    sendEvent(event, *event.handler);
}

void app::mouseWheel(mouseWheelEvent& event)
{
    sendEvent(event, *mouseOver_);
}

void app::windowSize(sizeEvent& event)
{
    sendEvent(event, *event.handler);
}

void app::windowPosition(positionEvent& event)
{
    sendEvent(event, *event.handler);
}

void app::windowDraw(drawEvent& event)
{
    sendEvent(event, *event.handler);
}

void app::destroyHandler(destroyEvent& event)
{
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

void app::error()
{
    if(settings_.onError == errorAction::Exit)
    {
        exit(-1);
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
        exit(-1);
        //throw e;
    }
}

//getMainApp
app* getMainApp()
{
    return app::getMainApp();
}


}
