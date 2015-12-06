#include <ny/font.hpp>

#include <ny/error.hpp>

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

}
