#pragma once

#include <ny/include.hpp>

#include <string>

namespace ny
{

class texture
{
public:
    void loadFromImage(image& img){ };
    bool loadFromFile(const std::string& name){ return 0; /* todo */ };
};


}
