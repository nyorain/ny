#pragma once

#include <ny/include.hpp>

#include <ny/surface.hpp>
#include <ny/file.hpp>

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
    bufferFormat getBufferFormat() const;

    unsigned int getStride() const;

    //from surface
    virtual vec2ui getSize() const override;

    //from file
    virtual bool loadFromFile(const std::string& path) override;
    virtual bool saveToFile(const std::string& path) const override;
};


//gif//////////////////////////////////////////
class gifImage : public file, public surface //maybe not surface
{
protected:
public:
    size_t getImageCount() const { return 0; }

    image* getImage(size_t i) { return nullptr; }
    image* operator[](size_t i) { return getImage(i); }

    //from surface
    virtual vec2ui getSize() const override { return vec2ui(); };

    //from file
    virtual bool loadFromFile(const std::string& path) override { return 0;};
    virtual bool saveToFile(const std::string& path) const override { return 0;};
};

}
