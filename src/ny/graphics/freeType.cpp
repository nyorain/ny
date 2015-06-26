#include "graphics/freeType.hpp"

#include "app/error.hpp"

namespace ny
{

FT_Library freeTypeFont::lib_ = nullptr;

/////
bool freeTypeFont::init()
{
    if(!lib_)
        return 1;

    int error = FT_Init_FreeType(&lib_);
    if(!error)
        return 1;

    //todo: proccess error
    return 0;
}

/////
freeTypeFont::freeTypeFont(const std::string& name, bool fromFile)
{
    std::string str = name;
    if(!fromFile)
    {
        str = "/usr/share/fonts/TTF/" + name;
    }

    int ftErr = FT_New_Face(lib_, str.c_str(), 0, &face_);

    if(ftErr)
    {
        throw error(error::Critical, "could lot load font");
        return;
    }
}

freeTypeFont::~freeTypeFont()
{
    if(face_) FT_Done_Face(face_);
}

}
