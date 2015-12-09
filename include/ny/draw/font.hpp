#pragma once

#include <ny/draw/include.hpp>
#include <nytl/cache.hpp>

namespace ny
{

///The Font class holds the settings for a specific font to load, and caches the backend handles.
class Font : public multiCache<std::string>
{
protected:
    bool fromFile_;
    std::string name_; //can conatain file name or fontname

public:
    Font(std::string name = "sans", bool fromFile = 0);

    void loadFromFile(const std::string& filename);
    void loadFromName(const std::string& fontname);

public:
    static Font& defaultFont()
    {
        static Font instance_;
        return instance_;
    }
};

}
