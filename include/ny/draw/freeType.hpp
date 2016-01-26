#pragma once

#include <ny/include.hpp>
#include <ny/draw/image.hpp>

#include <nytl/cache.hpp>
#include <nytl/cloneable.hpp>
#include <nytl/vec.hpp>

#include <string>
#include <memory>
#include <map>

typedef struct FT_LibraryRec_  *FT_Library;
typedef struct FT_FaceRec_*  FT_Face;

namespace ny
{

///Character
class Character
{
public:
	Image image;
	vec2i bearing {0, 0};
	unsigned int advance {0};
};

//Library
class FreeTypeLibrary
{
public:
	static FreeTypeLibrary& instance();

protected:
	FT_Library lib_;

public:
	FreeTypeLibrary();
	~FreeTypeLibrary();

	FT_Library handle() const { return lib_; }
};

//Cache Name: "ny::FreeTypeFontHandle"
class FreeTypeFontHandle : public deriveCloneable<cache, FreeTypeFontHandle>
{
protected:
    FT_Face face_ = nullptr;
	mutable std::map<char, Character> charCache_;

public:
    FreeTypeFontHandle(const Font& f);
    FreeTypeFontHandle(const std::string& name, bool fromFile = 0);
    ~FreeTypeFontHandle();

	FT_Face face() const { return face_; }
	void characterSize(const vec2ui& size);

	void cacheAscii() const;
	Character& load(char c) const;
};

}

