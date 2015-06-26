#pragma once

#include "include.hpp"

#include "utils/vec.hpp"

namespace ny
{

//buffer
enum class bufferFormat
{
    Unknown = 0,

    argb8888,
    rgba8888,
    rgb888,
    bit
};

class surface
{
public:
    virtual vec2ui getSize() const = 0;
};

}
