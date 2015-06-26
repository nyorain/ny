#include <ny/graphics/font.hpp>

#include <ny/graphics/freeType.hpp>
#include <ny/graphics/cairo.hpp>
#include <ny/app/error.hpp>

#ifdef WithWinapi
#include <ny/backends/winapi/gdi.hpp>
#endif //Winapi

namespace ny
{

font font::defaultFont;

font::~font()
{
    if(ftFont_) delete ftFont_;
    //if(cairoFont_) delete cairoFont_;

    #ifdef WithWinapi
    if(gdiFont_) delete gdiFont_;
    #endif //Winapi
}

void font::loadFromFile(const std::string& flename)
{
    fromFile_ = 1;
}

void font::loadFromName(const std::string& fontname)
{
    fromFile_ = 0;
}

freeTypeFont* font::getFreeTypeHandle(bool cr)
{
    if(cr && !ftFont_ && !name_.empty())
    {
        try
        {
            ftFont_ = new freeTypeFont(name_, fromFile_);
        }
        catch(error)
        {
            delete ftFont_;
            ftFont_ = nullptr;
            return nullptr;
        }
    }

    return ftFont_;
}

cairoFont* font::getCairoHandle(bool cr)
{
    //if(cr && !cairoFont_ && !name_.empty())
        //cairoFont_ = new cairoFont_(name_, fromFile_);

    return cairoFont_;
}

#ifdef WithWinapi
gdiFont* font::getGDIHandle(bool cr)
{
    if(cr && !gdiFont_ && !name_.empty())
        gdiFont_ = new gdiFont(name_, fromFile_);

    return gdiFont_;
}
#endif // Winapi

}
