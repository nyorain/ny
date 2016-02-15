#pragma once

#include <ny/include.hpp>
#include <ny/base/file.hpp>
#include <ny/draw/drawContext.hpp>
#include <ny/draw/image.hpp>

#include <memory>

namespace ny
{

///Implements the DrawContext interface to draw on an svg image surface.
///It basically just stores the drawn shapes in the SvgImage object it is related to.
class SvgDrawContext : public DelayedDrawContext
{
protected:
    SvgImage* image_ = nullptr;

public:
    SvgDrawContext(SvgImage& img);
    virtual ~SvgDrawContext() = default;

    SvgImage& svgImage() const { return *image_; }
    void svgImage(SvgImage& image){ image_ = &image; }

	virtual void paint(const Brush& alphaMask, const Brush& brush) override;

	virtual void fillPreserve(const Brush& col) override;
	virtual void strokePreserve(const Pen& col) override;

	virtual void clipRectangle(const Rect2f& Rectangle) override;
	virtual Rect2f rectangleClip() const override;
	virtual void resetRectangleClip() override;
};

//svg
class SvgImage : public File
{
protected:
    Vec2ui size_;
    std::vector<Shape> shapes_;

public:
    SvgImage(Vec2ui size) : size_(size) {}
    ~SvgImage() = default;

    Vec2ui size() const { return size_; };

    void addShape(const Shape& shape);
    const std::vector<Shape>& shapes() const { return shapes_; }

    ///Renders itself with the given size on the given DrawContext.
    void draw(DrawContext& dc);

    //from file
    virtual bool load(const std::string& path) override;
    virtual bool save(const std::string& path) const override;
};

}
