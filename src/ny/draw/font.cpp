#include <ny/draw/font.hpp>

namespace ny
{

Font& Font::defaultFont()
{
    static Font instance_("consola");
    return instance_;
}

Font::Font(const std::string& name, bool fromFile)
	: name_(name), fromFile_(fromFile)
{

}

void Font::loadFromFile(const std::string& filename)
{
	if(name_ == filename && fromFile_) return;

    name_ = filename;
    fromFile_ = 1;
	invalidateCache();
}

void Font::loadFromName(const std::string& fontname)
{
	if(name_ == fontname && !fromFile_) return;

    name_ = fontname;
    fromFile_ = 0;
	invalidateCache();
}

}
