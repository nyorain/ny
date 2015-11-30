#pragma once

#include <ny/file.hpp>
#include <ny/surface.hpp>
#include <ny/drawContext.hpp>
#include <ny/image.hpp>

#include <memory>

namespace ny
{

class svgDrawContext : public drawContext
{
protected:
public:
};

//svg/////////////////////////////////////////////
class svgImage : public file, public surface //maybe not surface
{
protected:
    std::unique_ptr<svgDrawContext> dc_;

public:
    svgImage(){}
    ~svgImage(){}

    svgDrawContext& getDrawContext() const { return *dc_; }
    svgDrawContext& getDC() const { return *dc_; }

    //from surface
    virtual vec2ui getSize() const override {};

    //from file
    virtual bool loadFromFile(const std::string& path) override {};
    virtual bool saveToFile(const std::string& path) const override {};
};

}
