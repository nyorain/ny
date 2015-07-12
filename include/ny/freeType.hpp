#pragma once

#include <string>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

namespace ny
{

class freeTypeFont
{
//static
protected:
    static FT_Library lib_;
public:
    static bool init();
    static FT_Library getLib(){ return lib_; }

//individual
protected:
    FT_Face face_;

public:
    freeTypeFont(const std::string& name, bool fromFile = 0);
    ~freeTypeFont();
};

}

