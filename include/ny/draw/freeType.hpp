#pragma once

#include <ny/draw/include.hpp>

#ifdef NY_WithFreetype
#include <nytl/cache.hpp>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <string>

namespace ny
{

//Cache Name: "ny::FTFontHandle"
class FTFontHandle : public cache
{
protected:
    static FT_Library lib_;

public:
    static bool init();
    static FT_Library getLib(){ return lib_; }

protected:
    FT_Face face_;

public:
    FreeTypeFontHandle(const std::string& name, bool fromFile = 0);
    ~FreeTypeFontHandle();
};

}

#endif //WithFreetype
