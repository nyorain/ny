#include "util.hpp"

#include "../include/color.hpp"

namespace ny
{

Color colorToWinapi(const color& col)
{
    return Color(col.a, col.r, col.g, col.b);
}

}
