#pragma once

#include <ny/file.hpp>
#include <ny/drawContext.hpp>
#include <ny/image.hpp>

#include <memory>

namespace ny
{

///Implements the DrawContext interface to draw on an svg image surface.
///It basically just stores the drawn shapes in the SvgImage object it is related to.
class SvgDrawContext : public DrawContext
{
protected:
    SvgImage* image_ = nullptr;

public:
    SvgDrawContext(SvgImage& img);
    virtual ~SvgDrawContext() = default;

    SvgImage& svgImage() const { return image_; }
    void svgImage(SvgImage& image){ image_ = &image; }
};

//svg/////////////////////////////////////////////
class SvgImage : public File
{
protected:
    vec2ui size_;
    std::vector<Shape> shapes_;

public:
    SvgImage(vec2ui size) : size_(size) {}
    ~SvgImage() = default;

    vec2ui size() const { return size_; };

    void addShape(const Shape& shape);
    const std::vector<Shape>& shapes() const { return shapes_; }

    ///Renders itself with the given size on the given DrawContext.
    void draw(DrawContext& dc, const vec2ui& size);

    //from file
    virtual bool loadFromFile(const std::string& path) override {};
    virtual bool saveToFile(const std::string& path) const override {};
};

}
