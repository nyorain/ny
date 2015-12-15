#include <ny/draw/font.hpp>

namespace ny
{

Font::Font(const std::string& name, bool fromFile)
	: name_(name), fromFile_(fromFile)
{
}

void Font::loadFromFile(const std::string& filename)
{
    name_ = filename;
    fromFile_ = 1;
}

void Font::loadFromName(const std::string& fontname)
{
    name_ = fontname;
    fromFile_ = 0;
}

}
