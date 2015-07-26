#include <ny/winapi/winapiUtil.hpp>

#include <ny/color.hpp>

namespace ny
{

Color colorToWinapi(const color& col)
{
    return Color(col.a, col.r, col.g, col.b);
}

}
