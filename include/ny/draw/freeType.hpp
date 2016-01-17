#pragma once

#include <ny/include.hpp>
#include <nytl/cache.hpp>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <string>

namespace ny
{

//Cache Name: "ny::FTFontHandle"
class FTFontHandle : public cacheBase<FTFontHandle>
{
protected:
    static FT_Library lib_;

public:
    static bool init();
    static FT_Library lib(){ return lib_; }

protected:
    FT_Face face_;

public:
    FTFontHandle(const std::string& name, bool fromFile = 0);
    ~FTFontHandle();

	FT_Face face() const { return face_; }
};

}

