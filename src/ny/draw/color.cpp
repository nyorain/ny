#include <ny/draw/color.hpp>

namespace ny
{

unsigned int Color::asInt()
{
    unsigned int ret = (a << 24) | (r << 16) | (g << 8) | (b);
    return ret;
}

void Color::normalized(float& pr, float& pg, float& pb, float& pa) const
{
    pr = r / 255.f;
    pg = g / 255.f;
    pb = b / 255.f;
    pa = a / 255.f;
}

void Color::normalized(float& pr, float& pg, float& pb) const
{
    pr = r / 255.f;
    pg = g / 255.f;
    pb = b / 255.f;
}

}
