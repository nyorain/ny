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
    if(position == pos) return;

    position = pos;
    moveCallback_(pos);
}

bool mouse::isButtonPressed(button b)
{
    if(b == button::none) return 0;
    return states[static_cast<unsigned int>(b)];
}

void mouse::buttonPressed(button b)
{
    if(b == button::none) return;
    states[static_cast<unsigned int>(b)] = 1;

    buttonCallback_(b, 1);
}

void mouse::buttonReleased(button b)
{
    if(b == button::none) return;
    states[static_cast<unsigned int>(b)] = 0;

    buttonCallback_(b, 0);
}

void mouse::wheelMoved(float value)
{
    wheelCallback_(value);
}

}
