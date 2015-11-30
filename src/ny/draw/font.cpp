#include <ny/font.hpp>

#include <ny/error.hpp>

#ifdef NY_WithFreeType
#include <ny/freeType.hpp>
#endif // NY_WithFreeType

#ifdef NY_WithCairo
#include <ny/cairo.hpp>
#endif // NY_WithCairo

#ifdef NY_WithWinapi
#include <ny/winapi/gdiDrawContext.hpp>
#endif //Winapi

namespace ny
{

font font::defaultFont;

void font::loadFromFile(const std::string& filename)
{
    name_ = filename;
    fromFile_ = 1;
}

void font::loadFromName(const std::string& fontname)
{
    name_ = fontname;
    fromFile_ = 0;
}

#ifdef NY_WithFreeType
freeTypeFont* font::getFreeTypeHandle(bool cr)
{
    if(cr && !ftFont_.get() && !name_.empty())
    {
        try
        {
            ftFont_ = make_unique<freeTypeFont>(name_, fromFile_);
        }
        catch(const std::exception& err)
        {
            nyWarning(err.what());
            ftFont_.reset();
        }
    }

    return ftFont_.get();
}
#endif

#ifdef NY_WithCairo
cairoFont* font::getCairoHandle(bool cr)
{
    if(cr && !cairoFont_.get() && !name_.empty())
    {
        try
        {
            cairoFont_ = make_unique<cairoFont>(name_, fromFile_);
        }
        catch(const std::exception& err)
        {
            nyWarning(err.what());
            cairoFont_.reset();
        }
    }

    return cairoFont_.get();
}
#endif

#ifdef NY_WithWinapi
gdiFont* font::getGDIHandle(bool cr)
{
    //if(cr && !gdiFont_ && !name_.empty())
        //sgdiFont_ = new gdiFont(name_, fromFile_);

    return gdiFont_.get();
}
#endif // Winapi

}
