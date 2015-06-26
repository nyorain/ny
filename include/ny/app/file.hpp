#pragma once

#include <string>

namespace ny
{

class file
{
protected:
    std::string m_filePath;

public:
    virtual bool saveToFile(std::string) const;
    virtual bool loadFromFile(std::string);
};

}
