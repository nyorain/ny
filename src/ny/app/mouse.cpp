#include <ny/app/mouse.hpp>

namespace ny
{

std::bitset<8> mouse::states;
vec2ui mouse::position;

vec2ui mouse::getPosition()
{
    return position;
}

void mouse::setPosition(int x, int y)
{
    position = vec2ui(x,y);
}

void mouse::setPosition(vec2i pos)
{
    position = pos;
}

bool mouse::isButtonPressed(button b)
{
    return states[(unsigned int) b];
}

void mouse::pressButton(button b)
{
    states[(unsigned int) b] = 1;
}

void mouse::releaseButton(button b)
{
    states[(unsigned int) b] = 0;
}

}
