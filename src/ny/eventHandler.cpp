#include <ny/eventHandler.hpp>

#include <ny/app.hpp>
#include <ny/error.hpp>
#include <ny/event.hpp>

#include <iostream>

namespace ny
{

eventHandler::eventHandler() : parent_(nullptr)
{
}

eventHandler::eventHandler(eventHandler& parent) : parent_(nullptr)
{
    create(parent);
}

eventHandler::~eventHandler()
{
    destroy();
}

void eventHandler::create(eventHandler& parent)
{
    parent_ = &parent;

    parent_->addChild(*this);
}

void eventHandler::reparent(eventHandler& newParent)
{
    if(!parent_)
        parent_->removeChild(*this);

    create(newParent);
}

void eventHandler::destroy()
{
    std::vector<eventHandler*> children = children_; //else there would be problems with removeChild
    children_.clear();

    for(unsigned int i(0); i < children.size(); i++)
    {
        children[i]->destroy();
    }

    if(parent_)
        parent_->removeChild(*this);

    parent_ = nullptr;
}

bool eventHandler::processEvent(event& event)
{
    if(event.type == eventType::destroy)
    {
        destroy();
        return true;
    }

    return false;
}

void eventHandler::addChild(eventHandler& child)
{
    children_.push_back(&child);
}

void eventHandler::removeChild(eventHandler& child)
{
    for(unsigned int i(0); i < children_.size(); i++)
    {
        if(children_[i] == &child)
        {
            children_.erase(children_.begin() + i);
            break;
        }
    }
}



}
