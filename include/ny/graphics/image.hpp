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

class imageHandle;

class image : public file, public surface
{
protected:
 unsigned char* data_;
 imageHandle* handle_;

public:
 image();
 image(const std::string& path);
 virtual ~image();

 virtual unsigned char* getData();
 const virtual unsigned char* getData() const;
 virtual void setData(unsigned char*);

 virtual void convert(bufferFormat){};

 virtual vec2ui getSize() const;
};

}
