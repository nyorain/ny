#pragma once

#include <ny/include.hpp>
#include <ny/gui/widget.hpp>

#include <nytl/vec.hpp>

namespace ny
{

class BoxSizer : public Widget
{
};

/*
{
protected:
    void cbResize(Vec2ui size)
    {
        if(sizerChildren_.empty())
            return;

        size_ = size;
        Vec2ui siz = size_;

        if(dir == sizerDir::horizontal) siz.x /= sizerChildren_.size();
        else siz.y /= sizerChildren_.size();

        for(unsigned int i(0); i < sizerChildren_.size(); i++)
        {
            Vec2ui pos = position_;
            if(dir == sizerDir::horizontal) pos.x += siz.x * i;
            else pos.y += siz.y * i;

            sizerChildren_[i]->processEvent(positionEvent(sizerChildren_[i], pos, 1));
            sizerChildren_[i]->processEvent(sizeEvent(sizerChildren_[i], siz, 1));
        }
    }

    Vec2ui size_;
    Vec2i position_;

    std::vector<eventHandler*> sizerChildren_;
    eventHandler* sizerParent_;

public:
    boxSizer(window& parent) : eventHandler(), sizerParent_(&parent)
    {
        parent.onResize(memberCallback(&boxSizer::cbResize, this));
    }
    template<sizerDir odir> boxSizer(boxSizer<odir>& parent) : eventHandler(), sizerParent_(&parent)
    {
        //parent.addChild(*this);
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

*/

}
