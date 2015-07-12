#include <ny/font.hpp>

#include <ny/error.hpp>

#ifdef NY_WithFreeType
#include <ny/freeType.hpp>
#endif // NY_WithFreeType

#ifdef NY_WithCairo
#include <ny/cairo.hpp>
#endif // NY_WithCairo

#ifdef NY_WithWinapi
#include <ny/winapi/gdi.hpp>
#endif //Winapi

#ifdef NY_WithWinapi
#include <ny/winapi/gdiDrawContext.hpp>
#endif // NY_WithWinapi

namespace ny
{

font font::defaultFont;

font::~font()
{
    #ifdef NY_WithFreeType
    if(ftFont_) delete ftFont_;
    #endif // NY_WithFreeType

    #ifdef NY_WithCairo
    if(cairoFont_) delete cairoFont_;
    #endif

    #ifdef NY_WithWinapi
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

#ifdef NY_WithFreeType
freeTypeFont* font::getFreeTypeHandle(bool cr)
{
    if(cr && !ftFont_ && !name_.empty())
    {
        try
        {
            ftFont_ = new freeTypeFont(name_, fromFile_);
        }
        catch(const std::exception& err)
        {
            sendWarning(err.what());

            delete ftFont_;
            ftFont_ = nullptr;
            return nullptr;
        }
    }

    return ftFont_;
}
#endif

#ifdef NY_WithCairo
cairoFont* font::getCairoHandle(bool cr)
{
    //if(cr && !cairoFont_ && !name_.empty())
        //cairoFont_ = new cairoFont_(name_, fromFile_);

    return cairoFont_;
}
#endif

#ifdef NY_WithWinapi
gdiFont* font::getGDIHandle(bool cr)
{
    if(cr && !gdiFont_ && !name_.empty())
        gdiFont_ = new gdiFont(name_, fromFile_);

    return gdiFont_;
}
#endif // Winapi

}
