#include <ny/base/cursor.hpp>

namespace ny
{

Cursor::Cursor(CursorType type) : type_(type), image_{}
{
}

Cursor::Cursor(const ImageData& img, const nytl::Vec2i& hotspot)
	: type_(CursorType::image), hotspot_(hotspot), image_(img)
{
}

void Cursor::image(const ImageData& img, const Vec2i& hotspot)
{
    image_ = img;
    hotspot_ = hotspot;

    type_ = CursorType::image;
}

void Cursor::nativeType(CursorType type)
{
    type_= type;
}

const ImageData* Cursor::image() const
{
    return (type_ == CursorType::image) ? &image_ : nullptr;
}

const Vec2i& Cursor::imageHotspot() const
{
    return hotspot_;
}

CursorType Cursor::type() const
{
    return type_;
}

}
