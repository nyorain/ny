#pragma once

#include <ny/include.hpp>
#include <bitset>

#include <nyutil/vec.hpp>


namespace ny
{


class mouse
{
protected:
    static std::bitset<8> states;
    static vec2ui position;

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

    static bool isButtonPressed(button b);

    static void pressButton(button b);
    static void releaseButton(button b);

    static vec2ui getPosition();
    static void setPosition(vec2i pos);
    static void setPosition(int x, int y);
};

}
