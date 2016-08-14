#include <ny/base/cursor.hpp>

namespace ny
{

Cursor::Cursor(CursorType type) : type_(type), image_(nullptr)
{
}

Cursor::Cursor(const Image& img, const nytl::Vec2i& hotspot)
	: type_(CursorType::image), image_(img), hotspot_(hotspot)
{
}

Cursor::Cursor(Image&& img, const nytl::Vec2i& hotspot)
	: type_(CursorType::image), image_(std::move(img)), hotspot_(hotspot)
{
}

void Cursor::image(const Image& img, const Vec2i& hotspot)
{
    image_ = img;
    hotspot_ = hotspot;

    type_ = CursorType::image;
}

void Cursor::image(Image&& img, const Vec2i& hotspot)
{
    image_ = std::move(img);
    hotspot_ = hotspot;

    type_ = CursorType::image;
}

void Cursor::nativeType(CursorType type)
{
    type_= type;
}

const Image* Cursor::image() const
{
    return (type_ == CursorType::image) ? &image_ : nullptr;
}

Image* Cursor::image()
{
    return (type_ == CursorType::image) ? &image_ : nullptr;
}

Vec2i Cursor::imageHotspot() const
{
    return hotspot_;
}

CursorType Cursor::type() const
{
    return type_;
}

}
