#pragma once

#include <string>

namespace ny
{

class file
{
protected:
    std::string filePath_;
	bool changed_;

public:
    virtual bool saveToFile(std::string) const = 0;
    virtual bool loadFromFile(std::string) = 0;
};

}
