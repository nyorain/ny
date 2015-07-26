#pragma once

#include <ny/include.hpp>

#include <ny/util/vec.hpp>

namespace ny
{

//buffer////////////
enum class bufferFormat : unsigned char
{
    Unknown = 0,

    argb8888,
    xrgb8888,
    rgba8888,
    rgb888,
    bit
};

unsigned getBufferFormatSize(bufferFormat format);

//surface///////////////
class surface
{
public:
    virtual vec2ui getSize() const = 0;
};

}
