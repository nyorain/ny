#include <ny/cursor.hpp>
#include <ny/image.hpp>

namespace ny
{

cursor::cursor(cursorType t) : type_(t), image_(nullptr)
{
}

cursor::cursor(const image& data) : type_(cursorType::image), image_(data)
{
}


void cursor::fromImage(const image& data)
{
    image_ = data;
    hotspot_ = - (data.getSize() / 2);

    type_ = cursorType::unknown;
}

void cursor::fromImage(const image& data, vec2i hotspot)
{
    image_ = data;
    hotspot_ = hotspot;

    type_ = cursorType::unknown;
}

void cursor::fromNativeType(cursorType t)
{
    type_= t;
}

bool cursor::isImage() const
{
    return (type_ == cursorType::image);
}

bool cursor::isNativeType() const
{
    return (type_ != cursorType::image);
}

const image* cursor::getImage() const
{
    return (type_ == cursorType::image) ? &image_ : nullptr;
}

vec2i cursor::getImageHotspot() const
{
    return hotspot_;
}

cursorType cursor::getNativeType() const
{
    return type_;
}

}
