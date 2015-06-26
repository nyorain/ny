#pragma once

#include <ny/include.hpp>

#include <ny/app/surface.hpp>
#include <ny/app/file.hpp>

#include <ny/graphics/drawContext.hpp>

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
