#include <ny/draw/color.hpp>

namespace ny
{

const Color Color::none{0, 0, 0, 0};
const Color Color::red{255, 0, 0};
const Color Color::green{0, 255, 0};
const Color Color::blue{0, 0, 255};
const Color Color::white{255, 255, 255};
const Color Color::black{0, 0, 0};


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
