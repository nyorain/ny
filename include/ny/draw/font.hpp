#pragma once

#include <ny/include.hpp>
#include <nytl/cache.hpp>

namespace ny
{

///The Font class holds the settings for a specific font to load, and caches the backend handles.
class Font : public MultiCache<std::string>
{
protected:
    std::string name_; //can contain file name or fontname
    bool fromFile_;

public:
    Font(const std::string& name = "sans", bool fromFile = 0);

    void loadFromFile(const std::string& filename);
    void loadFromName(const std::string& fontname);

	bool fromFile() const { return fromFile_; }
	const std::string& name() const { return name_; }

public:
    static Font& defaultFont()
    {
        static Font instance_("Ubuntu-M");
        return instance_;
    }
};

}
