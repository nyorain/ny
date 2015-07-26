#pragma once

#include <ny/include.hpp>

#include <ny/surface.hpp>
#include <ny/file.hpp>

#include <ny/drawContext.hpp>

#include <memory>

namespace ny
{


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

    virtual unsigned char* getDataConvent() const;
    virtual void getDataConvent(unsigned char* data) const;

    unsigned int getBufferSize() const;

    //drawing
    std::unique_ptr<drawContext> getDrawContext();

    //from surface
    vec2ui getSize() const;
    bufferFormat getBufferFormat() const;

    //from file
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;
};


}
