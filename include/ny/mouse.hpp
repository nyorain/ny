#pragma once

#include <ny/include.hpp>
#include <bitset>

#include <nyutil/vec.hpp>


namespace ny
{

struct mouseGrab
{
    eventHandler* grabber_ {nullptr};
    event* event_ {nullptr};
};

class mouse
{

friend class app;

public:
    enum button
    {
        none = -1,
        left = 0,
        right,
        middle,
        custom1,
        custom2,
        custom3,
        custom4,
    };

protected:
    static std::bitset<8> states;
    static vec2i position;

    static void buttonPressed(button b);
    static void buttonReleased(button b);
    static void setPosition(vec2i pos);

public:
    static bool isButtonPressed(button b);
    static vec2i getPosition();
};

}
