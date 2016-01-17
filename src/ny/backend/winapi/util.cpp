#include <ny/winapi/winapiUtil.hpp>
#include <ny/winapi/winapiAppContext.hpp>
#include <ny/app.hpp>

#include <ny/color.hpp>

namespace ny
{

Color colorToWinapi(const color& col)
{
    return Color(col.a, col.r, col.g, col.b);
}

winapiAppContext* nyWinapiAppContext()
{
    winapiAppContext* ret = nullptr;

    if(nyMainApp())
    {
        ret = dynamic_cast<winapiAppContext*>(nyMainApp()->getAppContext());
    }

    return ret;
}

winapiAppContext* nyWinapiAC()
{
    return nyWinapiAppContext();
}

}
