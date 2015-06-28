#pragma once

#include <ny/include.hpp>

#include <ny/app/surface.hpp>
#include <ny/app/file.hpp>

#include <ny/graphics/drawContext.hpp>

namespace ny
{

/*
enum class imageType
{
    Unknown = 0,

    JPG,
    PNG,
    GIF
};
*/

class imageHandle;

//image/////////////////////////////////////////////////////////
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

    vec2ui getSize() const;

    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;
};


}
