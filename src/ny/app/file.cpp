#include <ny/file.hpp>

namespace ny
{

file::file(const std::string& path) : filePath_(path)
{
}

bool file::load(const std::string& path)
{
    return 0;
}

bool file::save(const std::string& path) const
{
    return 1;
}

}
