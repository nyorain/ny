#include <ny/color.hpp>

namespace ny
{

const color color::red = color(255, 0, 0, 255);
const color color::green = color(0, 255, 0, 255);
const color color::blue = color(0, 0, 255, 255);
const color color::black = color(0, 0, 0, 255);
const color color::white = color(255, 255, 255, 255);
const color color::none = color(0, 0, 0, 0);

color::color(unsigned char rx, unsigned char gx, unsigned char bx, unsigned char ax) : r(rx), g(gx), b(bx), a(ax)
{
}

unsigned int color::toInt()
{
    unsigned int ret = (a << 24) | (r << 16) | (g << 8) | (b);
    return ret;
}

void color::normalized(float& pr, float& pg, float& pb, float& pa)
{
    pr = r / 255.f;
    pg = g / 255.f;
    pb = b / 255.f;
    pa = a / 255.f;
}

void color::normalized(float& pr, float& pg, float& pb)
{
    pr = r / 255.f;
    pg = g / 255.f;
    pb = b / 255.f;
}

}
