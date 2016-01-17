#include <ny/app/mouse.hpp>

namespace ny
{

std::bitset<8> Mouse::states_;
vec2i Mouse::position_;

callback<void(const vec2i&)> Mouse::moveCallback_;
callback<void(Mouse::Button, bool)> Mouse::buttonCallback_;
callback<void(float)> Mouse::wheelCallback_;

vec2i Mouse::position()
{
    return position_;
}

void Mouse::position(const vec2i& pos)
{
    if(all(position_ == pos)) return;

    position_ = pos;
    moveCallback_(pos);
}

bool Mouse::buttonPressed(Button b)
{
    if(b == Button::none) return 0;
    return states_[static_cast<unsigned int>(b)];
}

void Mouse::buttonPressed(Button b, bool pressed)
{
    if(b == Button::none) return;
    states_[static_cast<unsigned int>(b)] = pressed;

    buttonCallback_(b, pressed);
}

void Mouse::wheelMoved(float value)
{
    wheelCallback_(value);
}

}
