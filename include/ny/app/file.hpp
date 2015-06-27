#pragma once

#include <string>

namespace ny
{

class file
{
protected:
    std::string filePath_;
	bool changed_ = 0;

public:
	file() = default;
	file(const std::string& path);

    virtual bool saveToFile(std::string) const = 0;
    virtual bool loadFromFile(std::string) = 0;
};

}
