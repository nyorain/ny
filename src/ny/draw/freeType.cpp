#include <ny/draw/freeType.hpp>
#include <nytl/log.hpp>

namespace ny
{

FT_Library FTFontHandle::lib_ = nullptr;

/////
bool FTFontHandle::init()
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
FTFontHandle::FTFontHandle(const std::string& name, bool fromFile)
{
    std::string str = name;
    if(!fromFile)
    {
        str = "/usr/share/fonts/TTF/" + name;
    }

    int ftErr = FT_New_Face(lib_, str.c_str(), 0, &face_);

    if(ftErr)
    {
        throw std::runtime_error("could lot load font");
        return;
    }
}

FTFontHandle::~FTFontHandle()
{
    if(face_) FT_Done_Face(face_);
}

}
