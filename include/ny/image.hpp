#pragma once

#include <ny/include.hpp>

#include <ny/surface.hpp>
#include <ny/file.hpp>

#include <ny/drawContext.hpp>

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

//image/////////////////////////////
class image : public file, public surface
{
protected:
    unsigned char* data_;
    imageHandle* handle_;

public:
    image();
    image(const std::string& path);
    virtual ~image();

    virtual unsigned char* getDataPlain();
    const virtual unsigned char* getDataPlain() const;

    virtual unsigned char* getDataConvent();
    const virtual unsigned char* getDataConvent() const;

    imageType getType() const;

    //from surface
    vec2ui getSize() const;
    bufferFormat getBufferFormat() const;

    //from file
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;
};


}
