#pragma once

#include <ny/include.hpp>

#include <ny/surface.hpp>
#include <ny/file.hpp>

#include <ny/drawContext.hpp>

#include <memory>

namespace ny
{


//image/////////////////////////////
class image : public file, public surface
{
protected:
    class impl;
    std::unique_ptr<impl> impl_;

public:
    image();
    image(const std::string& path);
    virtual ~image();

    image(const image& other);
    image& operator=(const image& other);

    virtual unsigned char* getDataPlain();
    const virtual unsigned char* getDataPlain() const;

    virtual unsigned char* getData() const;
    virtual void getData(unsigned char* data) const;

    unsigned int getBufferSize() const;

    //from surface
    vec2ui getSize() const;
    bufferFormat getBufferFormat() const;

    unsigned int getStride() const;

    //from file
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;
};


}
