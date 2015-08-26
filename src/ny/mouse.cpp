#include <ny/mouse.hpp>

namespace ny
{

std::bitset<8> mouse::states;
vec2i mouse::position;

vec2i mouse::getPosition()
{
    return position;
}

void mouse::setPosition(vec2i pos)
{
    position = pos;
}

bool mouse::isButtonPressed(button b)
{
    return states[(unsigned int) b];
}

void mouse::buttonPressed(button b)
{
    states[(unsigned int) b] = 1;
}

void mouse::buttonReleased(button b)
{
    states[(unsigned int) b] = 0;
}

}
