#pragma once

#include "include.hpp"

#include "app/surface.hpp"
#include "app/file.hpp"

#include "graphics/drawContext.hpp"

namespace ny
{

enum class imageType
{
    Unknown = 0,

    JPG,
    PNG,
    GIF
};

class image : public file, public surface
{
protected:
 unsigned char* m_data;

public:
 image();
 virtual ~image();

 virtual unsigned char* getData() const;
 virtual void setData(unsigned char*);

 virtual void convert(bufferFormat);
};

}
