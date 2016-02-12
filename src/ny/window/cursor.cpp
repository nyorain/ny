#include <ny/window/cursor.hpp>
#include <ny/draw/image.hpp>

namespace ny
{

Cursor::Cursor(Type t) : type_(t), image_(nullptr)
{
}

Cursor::Cursor(const Image& data) : type_(Type::image), image_(data)
{
}


void Cursor::image(const Image& data)
{
    image_ = data;
    hotspot_ = -(data.size() / 2);

    type_ = Type::unknown;
}

void Cursor::image(const Image& data, const Vec2i& hotspot)
{
    image_ = data;
    hotspot_ = hotspot;

    type_ = Type::unknown;
}

void Cursor::nativeType(Type t)
{
    type_= t;
}

bool Cursor::isImage() const
{
    return (type_ == Type::image);
}

bool Cursor::isNativeType() const
{
    return (type_ != Type::image);
}

const Image* Cursor::image() const
{
    return (type_ == Type::image) ? &image_ : nullptr;
}

Vec2i Cursor::imageHotspot() const
{
    return hotspot_;
}

Cursor::Type Cursor::nativeType() const
{
    return type_;
}

}
