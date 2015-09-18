#pragma once

#include <ny/include.hpp>
#include <ny/eventHandler.hpp>
#include <ny/window.hpp>
#include <nyutil/misc.hpp>
#include <nyutil/vec.hpp>

namespace ny
{

enum class sizerDir : unsigned int
{
    vertical,
    horizontal
};

template<sizerDir dir>
class boxSizer : public eventHandler
{
protected:
    void cbResize(vec2ui size)
    {
        if(sizerChildren_.empty())
            return;

        size_ = size;
        vec2ui siz = size_;

        if(dir == sizerDir::horizontal) siz.x /= sizerChildren_.size();
        else siz.y /= sizerChildren_.size();

        for(unsigned int i(0); i < sizerChildren_.size(); i++)
        {
            vec2ui pos = position_;
            if(dir == sizerDir::horizontal) pos.x += siz.x * i;
            else pos.y += siz.y * i;

            sizerChildren_[i]->processEvent(positionEvent(sizerChildren_[i], pos, 1));
            sizerChildren_[i]->processEvent(sizeEvent(sizerChildren_[i], siz, 1));
        }
    }

    vec2ui size_;
    vec2i position_;

    std::vector<eventHandler*> sizerChildren_;
    eventHandler* sizerParent_;

public:
    boxSizer(window& parent) : eventHandler(), sizerParent_(&parent)
    {
        parent.onResize(memberCallback(&boxSizer::cbResize, this));
    }
    template<sizerDir odir> boxSizer(boxSizer<odir>& parent) : eventHandler(), sizerParent_(&parent)
    {
        parent.addChild(*this);
    }
    ~boxSizer() = default;

    virtual bool processEvent(const event& ev) override
    {
        if(eventHandler::processEvent(ev)) return 1;

        if(ev.type() == eventType::windowSize)
        {
            cbResize(event_cast<sizeEvent>(ev).size);
            return 1;
        }
        if(ev.type() == eventType::windowPosition)
        {
            position_ = event_cast<positionEvent>(ev).position;
            cbResize(size_);
            return 1;
        }

        return 0;
    }

    virtual void addChild(eventHandler& child)
    {
        sizerChildren_.push_back(&child);
        cbResize(size_);
    }
};

using spacer = eventHandler;
using hboxSizer = boxSizer<sizerDir::horizontal>;
using vboxSizer = boxSizer<sizerDir::vertical>;

}
