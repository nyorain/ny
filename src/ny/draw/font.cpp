#include <ny/draw/font.hpp>

namespace ny
{

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
